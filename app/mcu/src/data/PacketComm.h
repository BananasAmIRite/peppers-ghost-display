#pragma once

#include <stdint.h>
#include <functional>
#include "../DeviceState.h"
#include "Debug.h"
#include <HWCDC.h>

constexpr uint8_t SYNC_BYTE = 0x55;
constexpr uint8_t MAX_PAYLOAD = 64;

enum RxState
{
    WAIT_SYNC1,
    WAIT_SYNC2,
    READ_LEN,
    READ_TYPE,
    READ_PAYLOAD
};


typedef std::function<void(uint8_t, uint8_t*, uint8_t)> PacketHandler; 


class PacketComm {
    private:
            
        uint8_t len = 0;
        uint8_t type = 0;
        uint8_t data_index = 0;
        uint8_t buffer[MAX_PAYLOAD];
        
        RxState state = WAIT_SYNC1;
        CubeDevice* deviceHandler; 

        HWCDC& serial; 

    public: 
        PacketComm(HWCDC& ser): serial(ser) {}

        void loop() {
            
            while (serial.available())
            {
                processByte(serial.read());
            }
        }


        
        void registerCubeDevice(CubeDevice* handler) {
            deviceHandler = handler; 
        }


        void handlePacket(uint8_t type, uint8_t* data, uint8_t size) {
            LOG("Packet received: ");
            LOGLN(type); 
            deviceHandler->handlePacket(type, data, size); 
        }
            
        void processByte(uint8_t b) {
            switch (state)
            {
                case WAIT_SYNC1:
                    state = (b == SYNC_BYTE) ? WAIT_SYNC2 : WAIT_SYNC1;
                    break;

                case WAIT_SYNC2:
                    state = (b == SYNC_BYTE) ? READ_LEN : WAIT_SYNC1;
                    break;

                case READ_LEN:
                    len = b;

                    if (len == 0 || len > MAX_PAYLOAD + 1)
                        state = WAIT_SYNC1;
                    else
                        state = READ_TYPE;

                    break;

                case READ_TYPE:
                    type = b;
                    data_index = 0;

                    if (len == 1)
                    {
                        handlePacket(type, nullptr, 0);
                        state = WAIT_SYNC1;
                    }
                    else
                    {
                        state = READ_PAYLOAD;
                    }
                    break;

                case READ_PAYLOAD:
                    buffer[data_index++] = b;

                    if (data_index >= (len - 1))
                    {
                        handlePacket(type, buffer, len - 1);
                        state = WAIT_SYNC1;
                    }
                    break;
            }
        }


};