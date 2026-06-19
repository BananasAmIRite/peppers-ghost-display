#pragma once

#include "rendering/screens/MultipleScreen.h"
#include "rendering/screens/SlidingScreen.h"
#include "./RainScreen.h"
#include "./SunScreen.h"
#include "./CloudScreen.h"
#include "fonts/tiny512pt7b.h"
#include "utils/text_utils.h"
#include "data/UARTComms.h"
#include "packets.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>

enum WeatherCode {
    Clear = 0, 
    Cloudy = 1, 
    Drizzle = 2, 
    Rain = 3, 
    Storm = 4, 
    Snow = 5
};

struct WeatherState {
    float dailyMax; 
    float dailyMin;
    float currentTemp; 

    bool isNight; 
    
    WeatherCode currentWeatherCode; 
};


class WeatherOverlay : public Renderable {
    private: 
        WeatherState* state; 
    public:

        WeatherOverlay(WeatherState* weatherState): state(weatherState) {}
    
        void render(Adafruit_GFX* tft) override {
            int16_t width = tft->width();
            int16_t height = tft->height();

            tft->setFont(&Tiny5_Regular12pt7b);
            tft->setTextColor(COLOR_WHITE);
            tft->setTextSize(1); 

            if (!state) {
                drawCenteredText(tft, "No Weather Data", width / 2, height / 2);
                return;
            }

            char buffer[64];

            // Line 1: current temperature
            snprintf(buffer, sizeof(buffer),
                    "Now: %.1f F", state->currentTemp);
            drawCenteredText(tft, buffer, width / 2, height / 2 - 20);

            // Line 2: high / low
            snprintf(buffer, sizeof(buffer),
                    "H: %.1f F    L: %.1f F",
                    state->dailyMax,
                    state->dailyMin);
            drawCenteredText(tft, buffer, width / 2, height / 2 + 10);

            // Optional Line 3: weather code
            const char* weatherStr = "Unknown";

            switch (state->currentWeatherCode) {
                case WeatherCode::Clear:
                    weatherStr = "Clear";
                    break;
                case WeatherCode::Cloudy:
                    weatherStr = "Cloudy";
                    break;
                case WeatherCode::Drizzle:
                    weatherStr = "Drizzle";
                    break;
                case WeatherCode::Rain:
                    weatherStr = "Rainy";
                    break;
                case WeatherCode::Storm:
                    weatherStr = "Stormy";
                    break;
                case WeatherCode::Snow:
                    weatherStr = "Snowy";
                    break;
                default:
                    break;
            }

            drawCenteredText(tft, weatherStr, width / 2, height / 2 + 40);
        }
};

class WeatherScreen : public MultipleScreen, public UARTHandler {
    private:
        WeatherState state; 

        
        std::shared_ptr<SlidingScreen> background; 
        std::shared_ptr<WeatherOverlay> overlay; 

        // possible backgrounds
        std::shared_ptr<Renderable> clearScreen;  
        std::shared_ptr<Renderable> cloudyScreen;  // placeholder
        std::shared_ptr<Renderable> drizzleScreen; 
        std::shared_ptr<Renderable> rainScreen; 
        std::shared_ptr<Renderable> stormScreen;  
        std::shared_ptr<Renderable> snowScreen;  // placeholder


        void updateBackground() {
            WeatherCode code = state.currentWeatherCode; 

            background->setIndex(code); 
        }


    public: 
        WeatherScreen() : 
        MultipleScreen(), 
        state({0.0f, 0.0f, 0.0f, false, WeatherCode::Clear}), 
        background(std::make_shared<SlidingScreen>(false)), 
        overlay(std::make_shared<WeatherOverlay>(&state)),

        // screens
        clearScreen(std::make_shared<SunScreen>(&state.isNight)), 
        cloudyScreen(std::make_shared<CloudScreen>()), 
        drizzleScreen(std::make_shared<RainScreen>(300, 8, false)),
        rainScreen(std::make_shared<RainScreen>()),
        stormScreen(std::make_shared<RainScreen>(450, 64, true)), 
        snowScreen(std::make_shared<EmptyScreen>())
        {
            background->addScreen(clearScreen); // clear
            background->addScreen(cloudyScreen); // cloudy
            background->addScreen(drizzleScreen); // drizzling
            background->addScreen(rainScreen); // rain
            background->addScreen(stormScreen); // storm
            background->addScreen(snowScreen); // snow

            addScreen(background); 
            addScreen(overlay); 

            updateBackground();
        }

        void updateWeather(float dailyMax, float dailyMin, float curTemp, bool isNight, WeatherCode code) {
            state.dailyMax = dailyMax; 
            state.dailyMin = dailyMin; 
            state.currentTemp = curTemp; 
            state.currentWeatherCode = code; 

            state.isNight = isNight; 

            updateBackground(); 
        }

        void onActivate() override {
            background->onActivate(); 
            overlay->onActivate(); 
        }
        void onDeactivate() override {
            background->onDeactivate(); 
            overlay->onDeactivate(); 
        }

        
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len) override {
            
            if (type == WEATHER_SET) {
                // data format: float (32), float, float, uint8_t
                if (len < 14) return;
                // extract data and update
                uint32_t max_raw = ((uint32_t) data[0] << 24) | ((uint32_t) data[1] << 16) | ((uint32_t) data[2] << 8) | ((uint32_t) data[3] << 0); 
                uint32_t min_raw = ((uint32_t) data[4] << 24) | ((uint32_t) data[5] << 16) | ((uint32_t) data[6] << 8) | ((uint32_t) data[7] << 0); 
                uint32_t current_raw = ((uint32_t) data[8] << 24) | ((uint32_t) data[9] << 16) | ((uint32_t) data[10] << 8) | ((uint32_t) data[11] << 0); 
                float max, min, current; 
                memcpy(&max, &max_raw, sizeof(float)); 
                memcpy(&min, &min_raw, sizeof(float)); 
                memcpy(&current, &current_raw, sizeof(float)); 
                updateWeather(
                    max, 
                    min, 
                    current, 
                    data[12] & 0x01, 
                    (WeatherCode) data[13]); 
            }
        }
};
