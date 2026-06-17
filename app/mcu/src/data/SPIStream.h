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
        int slaveStatusPin; 
        int masterStatusPin; 

        // current stream states
        SPIStreamState state = STREAM_IDLE; 


        uint8_t* rx_buf = nullptr; 
        
        uint8_t* psram_buf = nullptr; 
        uint32_t psram_offset = 0; 

        ESP32DMASPI::Slave slave;

        std::vector<SPIStreamHandler*> handlers; 

    public:
        SPIStream(int slaveStatus, int masterStatus) : slaveStatusPin(slaveStatus), masterStatusPin(masterStatus), handlers() {

        }

        ~SPIStream() {
            if (psram_buf) free(psram_buf);

            if (rx_buf) free(rx_buf);
        }

        void addHandler(SPIStreamHandler* handler) {
            handlers.push_back(handler); 
        }

        void begin(uint8_t spi_bus, int sck, int miso, int mosi, int ss) {
            pinMode(slaveStatusPin, OUTPUT); 
            pinMode(masterStatusPin, INPUT_PULLUP); 

            psram_buf = (uint8_t*)heap_caps_malloc(PSRAM_BUF_SIZE, MALLOC_CAP_SPIRAM);

            if (!psram_buf) {
                LOGLN("Failed to allocate PSRAM buffer");
                return;
            }
            
            rx_buf = slave.allocDMABuffer(RX_SIZE); 
            slave.setDataMode(SPI_MODE0);           
            slave.setMaxTransferSize(RX_SIZE);  
            slave.setQueueSize(1);         

            slave.begin(spi_bus, sck, miso, mosi, ss);

            digitalWrite(slaveStatusPin, HIGH); 

        }

        void loop() {
            switch (state) {
                case STREAM_IDLE: {
                    if (!digitalRead(masterStatusPin)) {
                    LOGLN("Read master low!");
                        // master low! start transaction
                        psram_offset = 0; 

                        // TODO: queue transaction
                        slave.queue(NULL, rx_buf, RX_SIZE); 
                        slave.trigger(); 

                        digitalWrite(slaveStatusPin, LOW); 
                        state = WAIT_MASTER_TRANSACTION; 
                    }
                    break; 
                }
                case WAIT_MASTER_TRANSACTION: {
                    LOGLN("Waiting for master transaction");
                    if (slave.hasTransactionsCompletedAndAllResultsReady(1)) {
                        digitalWrite(slaveStatusPin, HIGH); 

                        // process transaction data
                        size_t bytes = slave.numBytesReceived(); 
                        LOG("Received bytes: "); 
                        LOGLN(bytes); 

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
                    LOGLN("Waiting for master signal");
                    if (digitalRead(masterStatusPin)) {
                        LOGLN("Finished transaction"); 
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
                        LOGLN("Transaction still going!");
                        // TODO: queue DMA
                        slave.queue(NULL, rx_buf, RX_SIZE); 
                        slave.trigger(); 
                        
                        // pull pin low to signal ready
                        digitalWrite(slaveStatusPin, LOW); 

                        state = WAIT_MASTER_TRANSACTION; 
                    }
                    break; 
                }
            }
        }
}; 