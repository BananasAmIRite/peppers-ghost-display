#pragma once

#include <driver/spi_slave.h>
#include <driver/gpio.h>
#include <esp_heap_caps.h>
#include <cstring>
#include <vector>
#include "Debug.h"
#include <ESP32DMASPISlave.h>

// Abstract callback class
class SPIStreamHandler {
public:
    virtual void onSPIData(uint8_t type, uint32_t bodyLen, uint8_t* body) = 0;
    virtual ~SPIStreamHandler() = default;
};

#define PSRAM_BUF_SIZE 2 * 1024 * 1024
#define RX_SIZE 4096

enum SPIStreamState {
    STREAM_IDLE, 
    WAIT_MASTER_TRANSACTION, 
    WAIT_MASTER_SIGNAL
};

struct  __attribute__((packed)) SPIStreamPacket {
    uint8_t type; 
    uint32_t size; 
    // data after that
}; 

class SPIStream {
    private:

        // config
        spi_host_device_t bus; 
        int slaveStatusPin; 
        int masterStatusPin; 

        // current stream states
        SPIStreamState state = STREAM_IDLE; 


        uint8_t* rx_buf = nullptr; 
        
        uint8_t* psram_buf = nullptr; 
        uint32_t psram_offset = 0; 

        // ESP32DMASPI::Slave slave;
        spi_slave_transaction_t m_async_trans;

        std::vector<SPIStreamHandler*> handlers; 

    public:
        SPIStream(spi_host_device_t spi_bus, int slaveStatus, int masterStatus) : bus(spi_bus), slaveStatusPin(slaveStatus), masterStatusPin(masterStatus), handlers() {

        }

        ~SPIStream() {
            if (psram_buf) free(psram_buf);

            if (rx_buf) free(rx_buf);
        }

        void addHandler(SPIStreamHandler* handler) {
            handlers.push_back(handler); 
        }

        bool begin(int sck, int miso, int mosi, int ss) {
            pinMode(slaveStatusPin, OUTPUT); 
            pinMode(masterStatusPin, INPUT_PULLUP); 

            psram_buf = (uint8_t*)heap_caps_malloc(PSRAM_BUF_SIZE, MALLOC_CAP_SPIRAM);

            if (!psram_buf) {
                LOGLN("Failed to allocate PSRAM buffer");
                return false;
            }

            rx_buf = (uint8_t*)heap_caps_malloc(RX_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
            
            // rx_buf = slave.allocDMABuffer(RX_SIZE); 
            // slave.setDataMode(SPI_MODE0);           
            // slave.setMaxTransferSize(RX_SIZE);  
            // slave.setQueueSize(2);         

            // slave.begin(spi_bus, sck, miso, mosi, ss);


            spi_bus_config_t buscfg = {};
            buscfg.mosi_io_num   = mosi;
            buscfg.miso_io_num   = miso;
            buscfg.sclk_io_num   = sck;
            buscfg.quadwp_io_num = -1;
            buscfg.quadhd_io_num = -1;
            buscfg.max_transfer_sz = RX_SIZE;

            spi_slave_interface_config_t slvcfg = {};
            slvcfg.mode          = 0;
            slvcfg.spics_io_num  = ss;
            slvcfg.queue_size    = 2;
            slvcfg.flags         = 0;

            esp_err_t ret = spi_slave_initialize(bus, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
            if (ret != ESP_OK) {
                LOG("SPI Slave Init Failed: ");
                LOGLN(esp_err_to_name(ret));
                return false;
            }

            digitalWrite(slaveStatusPin, HIGH); 

            return true; 

        }

        void queueNextBuf() {
            memset(&m_async_trans, 0, sizeof(m_async_trans)); 
            m_async_trans.length = RX_SIZE * 8;
            m_async_trans.rx_buffer = rx_buf;
            m_async_trans.tx_buffer = nullptr; 
            spi_slave_queue_trans(bus, &m_async_trans, 0); 
        }

        void loop() {
            switch (state) {
                case STREAM_IDLE: {
                    if (!digitalRead(masterStatusPin)) {
                        // LOGLN("Read master low!");
                        // master low! start transaction
                        psram_offset = 0; 

                        // TODO: queue transaction
                        // slave.queue(NULL, rx_buf, RX_SIZE); 
                        // slave.trigger(); 
                        queueNextBuf(); 

                        digitalWrite(slaveStatusPin, LOW); 
                        state = WAIT_MASTER_TRANSACTION; 
                    }
                    break; 
                }
                case WAIT_MASTER_TRANSACTION: {
                    // LOGLN("Waiting for master transaction");

                    spi_slave_transaction_t* completed_trans = nullptr;
                    esp_err_t ret = spi_slave_get_trans_result(bus, &completed_trans, 0);
                    if (ret == ESP_ERR_TIMEOUT) return;

                    if (ret != ESP_OK) {
                        LOG("SPI Async Result Error: ");
                        LOGLN(esp_err_to_name(ret));
                        return;
                    } else {
                        // transaction successful
                        digitalWrite(slaveStatusPin, HIGH); 

                        // process transaction data
                        size_t bytes = completed_trans->trans_len / 8;
                        // LOG("Received bytes: "); 
                        // LOGLN(bytes); 

                        if (psram_offset + bytes > PSRAM_BUF_SIZE) {
                            LOGLN("PSRAM overflow");

                            digitalWrite(slaveStatusPin, HIGH);

                            state = STREAM_IDLE;
                            return;
                        }
                        
                        memcpy(psram_buf+psram_offset, rx_buf, bytes);
                        psram_offset += bytes; 

                        state = WAIT_MASTER_SIGNAL; 
                    }
                    break; 
                }
                case WAIT_MASTER_SIGNAL: {
                    // LOGLN("Waiting for master signal");
                    if (digitalRead(masterStatusPin)) {
                        // LOGLN("Finished transaction"); 
                        // master pulled high, we're done!
                        
                        SPIStreamPacket packet;
                        if (psram_offset >= sizeof(packet)) { // otherwise throw away packet
                            memcpy(&packet, psram_buf, sizeof(packet)); 
                            
                            LOGLN(packet.type);
                            LOGLN(packet.size); 
                            uint32_t payloadReceived = psram_offset - sizeof(packet);
                            
                            LOGLN(payloadReceived);
                            if (packet.size == payloadReceived) {

                                for (auto* handler : handlers) {
                                    // TODO: do this
                                    handler->onSPIData(packet.type, packet.size, psram_buf + sizeof(packet)); 
                                }
                            }
                            else {
                                LOGLN("Incomplete packet");
                            }
                            
                        } else {
                            LOGLN("Malformed packet!"); 
                        }
                        // throw away psram now
                        // memset(psram_buf, 0, PSRAM_BUF_SIZE); 
                        digitalWrite(slaveStatusPin, HIGH);
                        state = STREAM_IDLE; 
                    } else {
                        // LOGLN("Transaction still going!");
                        // TODO: queue DMA
                        queueNextBuf(); 
                        // slave.queue(NULL, rx_buf, RX_SIZE); 
                        // slave.trigger(); 
                        
                        // pull pin low to signal ready
                        digitalWrite(slaveStatusPin, LOW); 

                        state = WAIT_MASTER_TRANSACTION; 
                    }
                    break; 
                }
            }
        }
}; 