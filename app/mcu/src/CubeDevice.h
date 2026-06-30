#pragma once
#include <stdint.h>
#include "rendering/Screen.h"
#include "rendering/screens/SlidingScreen.h"
#include "rendering/screens/cursor/CursorScreen.h"
#include "rendering/screens/MultipleScreen.h"
#include "rendering/screens/LoadingScreen.h"
#include "rendering/screens/chicken/ChickenScreen.h"
#include "rendering/screens/weather/WeatherScreen.h"
#include "rendering/screens/tasks/TasksScreen.h"
#include "data/UARTComms.h"
#include "data/SPIStream.h"
#include "rendering/screens/spotify/SpotifyScreen.h"
#include "types/packets.h"
#include "types/device_screen.h"



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

struct NextScreen {
    DeviceScreen next; 
    long transitionStart; 
};

#define SCREEN_TRANSITION_TIME 500



// representation of the device's overall state machine
// also handles packets
class CubeDevice : public UARTHandler, public SPIStreamHandler {
    private:
        // DeviceState curState = STARTUP; 
        DeviceScreen curScreen = STARTUP; 
        NextScreen nextScreen = {DeviceScreen::NONE, false}; 
        
        Screen* screenPtr; 

        UARTComms* packetReceiverPtr; 
        UARTComms* packetReceiver2Ptr; 
        SPIStream* spiStreamPtr; 

        
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
        TasksScreen tasksScreen; 

        // temporary screens
        SpotifyScreen spotifyScreen; 



    public: 
        CubeDevice(Screen* scrnPtr, Adafruit_ImageReader* reader, UARTComms* packetReceiver, UARTComms* packetReceiver2, SPIStream* spiReceiver) : 
            screenPtr(scrnPtr), 
            loadingScreen(333), 
            idleScreen(), 
            weatherScreen(), 
            tasksScreen(), 
            spotifyScreen(), 
            packetReceiverPtr(packetReceiver), 
            packetReceiver2Ptr(packetReceiver2), 
            spiStreamPtr(spiReceiver)
             {

                // uart and spi handlers
                spiStreamPtr->addHandler(&spotifyScreen); 
                packetReceiver->addUARTHandler(&spotifyScreen); 
                packetReceiver2->addUARTHandler(&spotifyScreen); 

                packetReceiver->addUARTHandler(&cursorScreen); 
                packetReceiver2->addUARTHandler(&cursorScreen); 

                packetReceiver->addUARTHandler(&weatherScreen); 
                packetReceiver2->addUARTHandler(&weatherScreen); 

                spiStreamPtr->addHandler(&tasksScreen); 

                screenPtr->setAuxRenderer(&cursorScreen);
        }

        
        void loop() {
            // rendering

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

            // screen transitioning
            if (nextScreen.next != DeviceScreen::NONE) {
                long timePassed = millis() - nextScreen.transitionStart; 
                if (timePassed > SCREEN_TRANSITION_TIME / 2) {
                    // if (curScreen != nextScreen.next) ledcFade(6, 1024, 0, 2000); 
                    curScreen = nextScreen.next;
                }
                if (timePassed > SCREEN_TRANSITION_TIME) {
                    // finished transition
                    nextScreen.next = DeviceScreen::NONE; 
                }
                // apply pwm
                screenPtr->setPWMOutput(1023 * (0.5f * (sin(2*PI/SCREEN_TRANSITION_TIME * timePassed) + 1)));
            } else {
                screenPtr->setPWMOutput(1023);
            }

            
            screenPtr->tryRender(); 
        }

        void setScreen(DeviceScreen newScreen) {
            // curScreen = newScreen; 
            if (newScreen == curScreen) return; 
            nextScreen.next = newScreen; 
            nextScreen.transitionStart = millis();
            
            // ledcFade(6, 1024, 0, 2000);  
        }



        
        // link handlePacket to packet comm
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len) override {
            switch (type) {

                case SET_SCREEN: 
                    if (len < 1) return;
                    setScreen((DeviceScreen) data[0]); 
                    break;




                case CURSOR_CLICK:
                    if (cursorScreen.getCursor()->cursorVisible) screenPtr->getCurrentRenderer()->click(screenPtr->getScreen(), cursorScreen.getCursor()->cursorX, cursorScreen.getCursor()->cursorY); 
                    break; 

                default:
                    break; 
            }
        }

        void onSPIData(uint8_t type, uint32_t length, uint8_t* body) override {

        }

}; 