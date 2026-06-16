#pragma once

#include <driver/spi_slave.h>
#include <driver/gpio.h>
#include <esp_heap_caps.h>
#include <cstring>
#include <vector>
#include "Debug.h"

// Abstract callback class
class SPIHandler {
public:
    virtual void onSPIData(uint8_t type, uint16_t metadataSize, uint8_t* metadata, uint32_t dataSize, uint8_t* data) = 0;
    virtual ~SPIHandler() = default;
};

class SPIComms {
public:
    enum class State {
        WAITING_FOR_HEADER,
        WAITING_FOR_METADATA,
        WAITING_FOR_BODY_CHUNK
    };

#pragma pack(push, 1)
    struct Header {
        uint8_t type;
        uint16_t metadata_len;
        uint32_t body_len;
    };
#pragma pack(pop)

private:
    // Hardware configuration
    spi_host_device_t m_host;
    int m_mosi, m_miso, m_cs, m_sclk;
    size_t m_chunk_size;
    
    // Bounds constraints
    size_t m_max_metadata;
    size_t m_max_body;
    
    std::vector<SPIHandler*> m_handlers;

    // Session Protocol State Machine
    State m_state;
    Header m_current_header;
    
    // Permanent, static PSRAM buffers
    uint8_t* m_metadata_buf;
    uint16_t m_metadata_received;
    
    uint8_t* m_body_buf;
    uint32_t m_body_received;

    // Internal Async & DMA safe buffers (Internal SRAM)
    uint8_t* m_ack_buf;
    uint8_t* m_rx_scratch;
    
    spi_slave_transaction_t m_async_trans;
    bool m_transaction_queued;

    void sendACK() {
        m_ack_buf[0] = 0xFF;
        spi_slave_transaction_t tx = {};
        tx.length = 8; 
        tx.tx_buffer = m_ack_buf;
        tx.rx_buffer = nullptr;
        
        esp_err_t ret = spi_slave_transmit(m_host, &tx, portMAX_DELAY);
        if (ret != ESP_OK) {
            LOGLN("Failed to send SPI ACK");
        }
    }

    // Allocation helper used exclusively at startup
    void* allocatePermanentPSRAM(size_t size) {
        if (size == 0) return nullptr;

        size_t alignment = 16; 
        size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

        void* ptr = heap_caps_aligned_alloc(alignment, aligned_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!ptr) {
            LOG("CRITICAL: Failed to allocate permanent "); 
            LOG(size); 
            LOGLN(" bytes in PSRAM!");
        }
        return ptr;
    }

    void resetSession() {
        m_state = State::WAITING_FOR_HEADER;
        m_metadata_received = 0;
        m_body_received = 0;
        std::memset(&m_current_header, 0, sizeof(Header));
        // NOTE: m_metadata_buf and m_body_buf are NEVER freed here anymore.
    }

    void invokeHandlers() {
        for (SPIHandler* handler : m_handlers) {
            if (handler != nullptr) {
                handler->onSPIData(
                    m_current_header.type, 
                    m_current_header.metadata_len, 
                    m_metadata_buf, 
                    m_current_header.body_len, 
                    m_body_buf
                );
            }
        }
    }

    bool queueNextTransaction() {
        if (m_transaction_queued) return true;

        std::memset(&m_async_trans, 0, sizeof(spi_slave_transaction_t));
        m_async_trans.rx_buffer = m_rx_scratch;
        m_async_trans.tx_buffer = nullptr;

        switch (m_state) {
            case State::WAITING_FOR_HEADER:
                m_async_trans.length = sizeof(Header) * 8;
                break;
            case State::WAITING_FOR_METADATA: {
                uint32_t remaining = m_current_header.metadata_len - m_metadata_received;
                uint32_t current_chunk_size = (remaining > m_chunk_size) ? m_chunk_size : remaining;
                m_async_trans.length = current_chunk_size * 8;
                break;
            }
            case State::WAITING_FOR_BODY_CHUNK: {
                uint32_t remaining = m_current_header.body_len - m_body_received;
                uint32_t current_chunk_size = (remaining > m_chunk_size) ? m_chunk_size : remaining;
                m_async_trans.length = current_chunk_size * 8;
                break;
            }
        }

        esp_err_t ret = spi_slave_queue_trans(m_host, &m_async_trans, 0); 
        if (ret == ESP_OK) {
            m_transaction_queued = true;
            return true;
        } else {
            LOG("SPI Slave Async Queue Failed: ");
            LOGLN(esp_err_to_name(ret));
            return false;
        }
    }

public:
    SPIComms(spi_host_device_t host, int mosi, int miso, int cs, int sclk, size_t chunkSize, size_t maxMetadata, size_t maxBody)
        : m_host(host), m_mosi(mosi), m_miso(miso), m_cs(cs), m_sclk(sclk), 
          m_chunk_size(chunkSize), m_max_metadata(maxMetadata), m_max_body(maxBody),
          m_state(State::WAITING_FOR_HEADER),
          m_metadata_buf(nullptr), m_metadata_received(0), m_body_buf(nullptr), m_body_received(0),
          m_rx_scratch(nullptr), m_transaction_queued(false)
    {
        // Allocate hardware-tied scratch buffers in internal SRAM
        m_ack_buf = (uint8_t*)heap_caps_malloc(4, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        m_rx_scratch = (uint8_t*)heap_caps_malloc(m_chunk_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

        // Pre-allocate the massive structural blocks permanently inside PSRAM right at boot up
        m_metadata_buf = (uint8_t*)allocatePermanentPSRAM(m_max_metadata);
        m_body_buf = (uint8_t*)allocatePermanentPSRAM(m_max_body);
    }

    ~SPIComms() {
        resetSession();
        if (m_ack_buf) free(m_ack_buf);
        if (m_rx_scratch) free(m_rx_scratch);
        
        // Permanent buffers are only torn down if the wrapper instance lifecycle ends
        if (m_metadata_buf) heap_caps_aligned_free(m_metadata_buf);
        if (m_body_buf) heap_caps_aligned_free(m_body_buf);
        
        spi_slave_free(m_host);
    }

    void addHandler(SPIHandler* handler) {
        if (handler) {
            m_handlers.push_back(handler);
        }
    }

    bool begin() {
        // Enforce fallback if allocations failed in the constructor
        if ((m_max_metadata > 0 && !m_metadata_buf) || (m_max_body > 0 && !m_body_buf)) {
            LOGLN("SPI Transport cannot begin due to missing PSRAM block allocations!");
            return false;
        }

        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = m_mosi;
        buscfg.miso_io_num = m_miso;
        buscfg.sclk_io_num = m_sclk;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;

        spi_slave_interface_config_t slvcfg = {};
        slvcfg.mode = 0; 
        slvcfg.spics_io_num = m_cs;
        slvcfg.queue_size = 2; 
        slvcfg.flags = 0;

        esp_err_t ret = spi_slave_initialize(m_host, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK) {
            LOG("SPI Slave Init Failed: "); 
            LOGLN(esp_err_to_name(ret));
            return false;
        }

        return queueNextTransaction();
    }

    void loop() {
        if (!m_transaction_queued) {
            queueNextTransaction();
            return;
        }

        spi_slave_transaction_t* completed_trans = nullptr;
        esp_err_t ret = spi_slave_get_trans_result(m_host, &completed_trans, 0);

        if (ret == ESP_ERR_TIMEOUT) {
            return; 
        }

        if (ret != ESP_OK) {
            LOG("SPI Async Result Error: ");
            LOGLN(esp_err_to_name(ret));
            m_transaction_queued = false;
            queueNextTransaction();
            return;
        }

        m_transaction_queued = false;
        size_t bytes_received = completed_trans->trans_len / 8;
        if (bytes_received == 0) {
            queueNextTransaction();
            return;
        }

        switch (m_state) {
            case State::WAITING_FOR_HEADER: {
                if (bytes_received >= sizeof(Header)) {
                    std::memcpy(&m_current_header, m_rx_scratch, sizeof(Header));
                    
                    LOG("Header Received. Type: "); LOG(m_current_header.type); 
                    LOG(", MetaLen: "); LOG(m_current_header.metadata_len);
                    LOG(", BodyLen: "); LOGLN(m_current_header.body_len); 

                    // --- BOUNDS CHECK SECURITY INJECTION ---
                    if (m_current_header.metadata_len > m_max_metadata || m_current_header.body_len > m_max_body) {
                        LOGLN("CRITICAL: Incoming Master packet lengths exceed hardware limits! Dropping session.");
                        sendACK(); // Keep master in sync, but break state sequence out safely
                        resetSession();
                        break;
                    }

                    sendACK();

                    if (m_current_header.metadata_len > 0) {
                        m_state = State::WAITING_FOR_METADATA;
                    } else if (m_current_header.body_len > 0) {
                        m_state = State::WAITING_FOR_BODY_CHUNK;
                    } else {
                        invokeHandlers();
                        resetSession();
                    }
                }
                break;
            }

            case State::WAITING_FOR_METADATA: {
                // Bounds enforcement confirmation fallback
                if (m_metadata_received + bytes_received <= m_max_metadata) {
                    std::memcpy(m_metadata_buf + m_metadata_received, m_rx_scratch, bytes_received);
                    m_metadata_received += bytes_received;
                }

                if (m_metadata_received >= m_current_header.metadata_len) {
                    sendACK();
                    if (m_current_header.body_len > 0) {
                        m_state = State::WAITING_FOR_BODY_CHUNK;
                    } else {
                        invokeHandlers();
                        resetSession();
                    }
                }
                break;
            }

            case State::WAITING_FOR_BODY_CHUNK: {
                // Bounds enforcement confirmation fallback
                if (m_body_received + bytes_received <= m_max_body) {
                    std::memcpy(m_body_buf + m_body_received, m_rx_scratch, bytes_received);
                    m_body_received += bytes_received;
                }

                sendACK(); 

                if (m_body_received >= m_current_header.body_len) {
                    LOGLN("Full packet assembled successfully. Broadcasting...");
                    invokeHandlers();
                    resetSession(); 
                }
                break;
            }
        }

        queueNextTransaction();
    }
};