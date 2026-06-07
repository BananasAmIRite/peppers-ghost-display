#pragma once

#include "../../MultipleScreen.h"
#include "../../SlidingScreen.h"
#include "./RainScreen.h"
#include "../../../../fonts/tiny512pt7b.h"
#include "../../../../utils/text_utils.h"
#include <Adafruit_ImageReader.h>
#include <memory>
#include <vector>
#include <string>
#include <utility>

enum WeatherCode {
    Clear = 0, 
    Cloudy = 1, 
    Rain = 2, 
    Storm = 3, 
    Snow = 4
};

struct WeatherState {
    float dailyMax; 
    float dailyMin;
    float currentTemp; 
    
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
                case WeatherCode::Rain:
                    weatherStr = "Rain";
                    break;
                case WeatherCode::Storm:
                    weatherStr = "Storm";
                    break;
                case WeatherCode::Snow:
                    weatherStr = "Snow";
                    break;
                default:
                    break;
            }

            drawCenteredText(tft, weatherStr, width / 2, height / 2 + 40);
        }
};

class WeatherScreen : public MultipleScreen {
    private:
        WeatherState state; 

        // possible backgrounds
        std::shared_ptr<Renderable> clearScreen;  // placeholder
        std::shared_ptr<Renderable> cloudyScreen;  // placeholder
        std::shared_ptr<Renderable> rainScreen; 
        std::shared_ptr<Renderable> stormScreen;  
        std::shared_ptr<Renderable> snowScreen;  // placeholder

        std::shared_ptr<SlidingScreen> background; 
        std::shared_ptr<WeatherOverlay> overlay; 

        void updateBackground() {
            WeatherCode code = state.currentWeatherCode; 

            background->setIndex(code); 
        }


    public: 
        WeatherScreen(Adafruit_ImageReader& reader) : 
        MultipleScreen(), 
        state(), 
        background(std::make_shared<SlidingScreen>(false)), 
        overlay(std::make_shared<WeatherOverlay>(&state)),

        // screens
        clearScreen(std::make_shared<EmptyScreen>()), 
        cloudyScreen(std::make_shared<EmptyScreen>()), 
        rainScreen(std::make_shared<RainScreen>(reader)),
        stormScreen(std::make_shared<RainScreen>(reader, 300, 64, true)), 
        snowScreen(std::make_shared<EmptyScreen>())
        {
            background->addScreen(clearScreen); // clear
            background->addScreen(cloudyScreen); // cloudy
            background->addScreen(rainScreen); // rain
            background->addScreen(stormScreen); // storm
            background->addScreen(snowScreen); // snow

            addScreen(background); 
            addScreen(overlay); 
        }

        void updateWeather(float dailyMax, float dailyMin, float curTemp, WeatherCode code) {
            state.dailyMax = dailyMax; 
            state.dailyMin = dailyMin; 
            state.currentTemp = curTemp; 
            state.currentWeatherCode = code; 

            updateBackground(); 
        }
};
