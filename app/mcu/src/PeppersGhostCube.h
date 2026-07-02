#pragma once

#include <stdint.h>
#include <string.h>
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

enum class TransitionPhase {
    NONE,
    FADING_OUT,
    WAITING_FOR_LOAD,
    HOLD_BLACK, 
    FADING_IN
};

struct NextScreen {
    DeviceScreen next; 
    long transitionStart; 
    TransitionPhase phase = TransitionPhase::NONE;
};

#define SCREEN_TRANSITION_TIME 500
#define SCREEN_LOAD_BUFFER_MS 200



// representation of the device's overall state machine
// also handles packets
class PeppersGhostCube : public UARTHandler, public SPIStreamHandler {
    private:
        // DeviceState curState = STARTUP; 
        volatile DeviceScreen curScreen = STARTUP; 
        volatile NextScreen nextScreen = {DeviceScreen::NONE, false}; 
        volatile bool screenReadyAfterLoad = false;
        
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

        void updatePWMState();

        void setScreen(DeviceScreen newScreen);



        
        
        void onUARTData(uint8_t type, uint8_t* data, uint8_t len, std::string name) override;

        void onSPIData(uint8_t type, uint32_t length, uint8_t* body) override;

        Screen* getScreen(); 

}; 