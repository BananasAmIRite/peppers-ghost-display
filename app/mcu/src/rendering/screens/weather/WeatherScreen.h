#pragma once

#include "rendering/screens/MultipleScreen.h"
#include "rendering/screens/SlidingScreen.h"
#include "data/UARTComms.h"

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

        WeatherOverlay(WeatherState* weatherState);
    
        void render(Adafruit_GFX* tft) override;
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
        WeatherScreen();

        void updateWeather(float dailyMax, float dailyMin, float curTemp, bool isNight, WeatherCode code);

        void onActivate() override {
            background->onActivate(); 
            overlay->onActivate(); 
        }
        void onDeactivate() override {
            background->onDeactivate(); 
            overlay->onDeactivate(); 
        }

        
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len, std::string name) override;
};
