#pragma once

#include "rendering/Screen.h"
#include "rendering/screens/SpriteScreen.h"
#include "data/UARTComms.h"
#include "data/SPIStream.h"
#include "types/packets.h"
#include "types/device_screen.h"

struct CalendarEvent {
    std::string name; 
    time_t timestampStart; 
    time_t timestampEnd; 
};


class CalendarScreen : public Renderable, public UARTHandler, public SPIStreamHandler {
    private:
        unsigned long long cal_timestamp = 0ULL; 
        int32_t cal_offset = 0; 
        unsigned long long last_message = 0; 
        
        std::vector<CalendarEvent> events; 

    public: 
        CalendarScreen();

        void render(Adafruit_GFX* tft) override;

        time_t computeCurTimestamp() {
            return cal_timestamp + (millis() - last_message) / 1000;
        }

        time_t computeCurTimestampLocal() {
            return cal_timestamp + cal_offset + (millis() - last_message) / 1000;
        }

        std::vector<CalendarEvent> getCurrentEvents();
        std::vector<CalendarEvent> getNextEvents(size_t count = 3);
        
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len, std::string name) override;
        void onSPIData(uint8_t type, uint32_t bodyLen, uint8_t* body) override; 
};