#pragma once

#include <stdint.h>
#include "data/UARTComms.h"
#include "data/SPIStream.h"
#include "types/packets.h"
#include "types/device_screen.h"

#include "rendering/Screen.h"
#include "rendering/screens/SlidingScreen.h"
#include "rendering/screens/MultipleScreen.h"
#include "rendering/screens/LoadingScreen.h"

#include "rendering/screens/cursor/CursorScreen.h"

#include "rendering/screens/chicken/ChickenScreen.h"
#include "rendering/screens/weather/WeatherScreen.h"
#include "rendering/screens/tasks/TasksScreen.h"
#include "rendering/screens/spotify/SpotifyScreen.h"



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
class PeppersGhostCube : public UARTHandler, public SPIStreamHandler {
    private:
        // DeviceState curState = STARTUP; 
        DeviceScreen curScreen = STARTUP; 
        NextScreen nextScreen = {DeviceScreen::NONE, false}; 
        
        Screen* screenPtr; 

        UARTComms* packetReceiverPtr; 
        UARTComms* packetReceiver2Ptr; 
        SPIStream* spiStreamPtr; 

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
        PeppersGhostCube(Screen* scrnPtr, Adafruit_ImageReader* reader, UARTComms* packetReceiver, UARTComms* packetReceiver2, SPIStream* spiReceiver);

        
        void loop();

        void setScreen(DeviceScreen newScreen);



        
        
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len) override;

        void onSPIData(uint8_t type, uint32_t length, uint8_t* body) override;

}; 