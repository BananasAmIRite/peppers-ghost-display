#pragma once

#include "../../MultipleScreen.h"
#include "../../SlidingScreen.h"
#include "./RainScreen.h"
#include "./SunScreen.h"
#include "../../../../fonts/tiny512pt7b.h"
#include "../../../../utils/text_utils.h"
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

class WeatherScreen : public MultipleScreen {
    private:
        WeatherState state; 

        // possible backgrounds
        std::shared_ptr<Renderable> clearScreen;  
        std::shared_ptr<Renderable> cloudyScreen;  // placeholder
        std::shared_ptr<Renderable> drizzleScreen; 
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
        WeatherScreen() : 
        MultipleScreen(), 
        state(), 
        background(std::make_shared<SlidingScreen>(false)), 
        overlay(std::make_shared<WeatherOverlay>(&state)),

        // screens
        clearScreen(std::make_shared<SunScreen>(&state.isNight)), 
        cloudyScreen(std::make_shared<EmptyScreen>()), 
        drizzleScreen(std::make_shared<RainScreen>(200, 8, false)),
        rainScreen(std::make_shared<RainScreen>()),
        stormScreen(std::make_shared<RainScreen>(300, 64, true)), 
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
};
