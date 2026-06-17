#pragma once
#include <stdint.h>
#include "rendering/Screen.h"
#include "rendering/screens/SlidingScreen.h"
#include "rendering/screens/cursor/CursorScreen.h"
#include "rendering/screens/MultipleScreen.h"
#include "rendering/screens/LoadingScreen.h"
#include "rendering/screens/chicken/ChickenScreen.h"
#include "rendering/screens/work/weather/WeatherScreen.h"
#include "data/UARTComms.h"
#include "data/SPIStream.h"
#include "rendering/screens/work/spotify/SpotifyScreen.h"


enum PacketType {
    CURSOR_SET = 0x10, 
    CURSOR_VISIBLE = 0x11, 
    CURSOR_CLICK = 0x12, 
    CURSOR_DOWN = 0x13, 
    CURSOR_UP = 0x14,
    
    SET_SCREEN = 0x20, 
    SET_SCREEN_NOTIF = 0x21, 

    WEATHER_SET = 0x30, 
    TASKS_ADD = 0x31, 
    CALENDAR_ADD = 0x32, 


    // debug
    DEBUG_SET_IMAGE = 0xF0
};

// /**
//  * State machine goes like this: 
//  * 
//  * STARTUP -> IDLE triggered by DEVICE_START
//  * IDLE -> WORK triggered by FOCUS
//  * WORK -> IDLE triggered by inactive timeout?
//  * 
//  */

// enum DeviceState {
//     STARTUP, 
//     IDLE, 
//     WORK
// }; 

enum DeviceScreen {
    STARTUP,
    // perm 
    IDLE, 
    WEATHER, 
    TASKS,

    // temp
    SPOTIFY, 
    MEETINGS
};


// representation of the device's overall state machine
// also handles packets
class CubeDevice : public UARTHandler, public SPIStreamHandler {
    private:
        // DeviceState curState = STARTUP; 
        DeviceScreen curScreen = STARTUP; 
        
        Screen* screenPtr; 

        
        // MultipleScreen loadingScreenComposed;
        // MultipleScreen idleScreenComposed; 
        // MultipleScreen workScreenComposed; 

        // // screens!
        // std::shared_ptr<LoadingScreen> loadingScreen; 
        // std::shared_ptr<ChickenScreen> idleScreen; 
        // std::shared_ptr<SlidingScreen> workScreen; 

        // // work screen stuff
        // std::shared_ptr<WeatherScreen> work_weatherScreen;  
        // std::shared_ptr<Renderable> work_tasksScreen; 

        CursorScreen cursorScreen; 

        // 
        LoadingScreen loadingScreen; 

        // permanent screens
        ChickenScreen idleScreen; 
        WeatherScreen weatherScreen; 
        EmptyScreen tasksScreen; 

        // temporary screens
        SpotifyScreen spotifyScreen; 



    public: 
        CubeDevice(Screen* scrnPtr, Adafruit_ImageReader* reader): 
            screenPtr(scrnPtr), 
            // loadingScreenComposed(), 
            // idleScreenComposed(), 
            // workScreenComposed(), 
            // cursorScreen(std::make_shared<CursorScreen>())
            loadingScreen(333), 
            idleScreen(), 
            weatherScreen(), 
            tasksScreen(), 
            spotifyScreen()
             {

                // init screens
                // loadingScreen = std::make_shared<LoadingScreen>(333); 
                // idleScreen = std::make_shared<ChickenScreen>(); 
                // work_weatherScreen = std::make_shared<WeatherScreen>();
                // work_tasksScreen = std::make_shared<EmptyScreen>();

                // workScreen = std::make_shared<SlidingScreen>(); 
                // workScreen->addScreen(work_weatherScreen); 
                // workScreen->addScreen(work_tasksScreen); 

                // loadingScreenComposed.addScreen(loadingScreen); 
                // idleScreenComposed.addScreen(idleScreen); 
                // workScreenComposed.addScreen(workScreen); 

                
                // loadingScreenComposed.addScreen(cursorScreen); 
                // idleScreenComposed.addScreen(cursorScreen); 
                // workScreenComposed.addScreen(cursorScreen); 
        }

        
        void loop() {
            // rendering
            // if (curState == STARTUP) {
            // screenPtr->setRenderer(&loadingScreenComposed); 
            // } else if (curState == IDLE) {
            //     screenPtr->setRenderer(&idleScreenComposed); 
            // } else if (curState == WORK) {
            //     screenPtr->setRenderer(&workScreenComposed); 
            // }

            switch (curScreen) {
                default:
                case STARTUP: 
                    screenPtr->setRenderer(&loadingScreen); 
                    break;
                case IDLE: 
                    screenPtr->setRenderer(&idleScreen); 
                    break; 
                case WEATHER: 
                    screenPtr->setRenderer(&weatherScreen); 
                    break; 
                case TASKS: 
                    screenPtr->setRenderer(&tasksScreen); 
                    break; 
                case SPOTIFY: 
                    screenPtr->setRenderer(&spotifyScreen); 
                    break;
            }

            
            screenPtr->tryRender(); 
        }

        void setScreen(DeviceScreen newScreen) {
            curScreen = newScreen; 
        }



        
        // link handlePacket to packet comm
        void handlePacket(uint8_t type, uint8_t* data, uint8_t len) {
            switch (type) {

                case SET_SCREEN: 
                    if (len < 1) return; 
                    setScreen((DeviceScreen) data[0]); 
                    break;

                case SET_SCREEN_NOTIF:
                    // TODO: implement
                    break; 



                case CURSOR_SET:
                    if (len < 4) return; 
                    cursorScreen.getCursor()->cursorX = data[0] << 8 | data[1]; 
                    cursorScreen.getCursor()->cursorY = data[2] << 8 | data[3]; 
                    break;

                case CURSOR_VISIBLE:
                    if (len < 1) return; 
                    cursorScreen.getCursor()->cursorVisible = data[0] & 0x01; 
                    break; 
                case CURSOR_CLICK:
                    if (cursorScreen.getCursor()->cursorVisible) screenPtr->getCurrentRenderer()->click(screenPtr->getScreen(), cursorScreen.getCursor()->cursorX, cursorScreen.getCursor()->cursorY); 
                    break; 
                case CURSOR_DOWN: 
                    cursorScreen.getCursor()->mouseDown = true; 
                    break; 
                case CURSOR_UP:
                    cursorScreen.getCursor()->mouseDown = false; 
                    break; 

                case WEATHER_SET: {
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
                    weatherScreen.updateWeather(
                        max, 
                        min, 
                        current, 
                        data[12] & 0x01, 
                        (WeatherCode) data[13]); 
                    }
                break; 
                case TASKS_ADD:
                break; 
                case CALENDAR_ADD:
                break;
            }
        }

        void onSPIData(uint8_t type, uint32_t length, uint8_t* body) override {
            LOGLN(type); 
            // if (type == DEBUG_SET_IMAGE) {
            //     // if (metadataSize < 4) return; 
            //     // uint16_t width = ((uint16_t)metadata[1] << 8) | metadata[0]; 
            //     // uint16_t height = ((uint16_t)metadata[3] << 8) | metadata[2];
                
            //     LOG(type); 

            //     // LOG("WIDTH: ");
            //     // LOG(width); 
            //     // LOG(", HEIGHT: "); 
            //     // LOGLN(height); 

            //     // spotifyScreen.loadBuffer(width, height, data, dataSize);


            // }
        }

}; 