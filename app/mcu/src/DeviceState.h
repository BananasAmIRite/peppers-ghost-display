#pragma once
#include <stdint.h>
#include "./rendering/Screen.h"
#include "./rendering/screens/SlidingScreen.h"
#include "rendering/screens/cursor/CursorScreen.h"
#include "rendering/screens/MultipleScreen.h"


typedef enum PacketType {
    DEVICE_START = 0x01, 

    CURSOR_SET = 0x10, 
    CURSOR_VISIBLE = 0x11, 
    CURSOR_CLICK = 0x12, 
    CURSOR_DOWN = 0x13, 
    CURSOR_UP = 0x14,
    
    SCROLL_LEFT = 0x20, 
    SCROLL_RIGHT = 0x21, 
    FOCUS = 0x22, 

    WEATHER = 0x30, 
    TASKS_ADD = 0x31, 
    CALENDAR_ADD = 0x32
};

/**
 * State machine goes like this: 
 * 
 * STARTUP -> IDLE triggered by DEVICE_START
 * IDLE -> WORK triggered by FOCUS
 * WORK -> IDLE triggered by inactive timeout?
 * 
 */

typedef enum DeviceState {
    STARTUP, 
    IDLE, 
    WORK
}; 


// representation of the device's overall state machine
// also handles packets
class CubeDevice {
    private:
        DeviceState curState; 
        
        Screen* screenPtr; 

        CursorState cursorState; 
        
        MultipleScreen loadingScreenComposed;
        MultipleScreen idleScreenComposed; 
        MultipleScreen workScreenComposed; 

        std::shared_ptr<Renderable> loadingScreen; 
        std::shared_ptr<Renderable> idleScreen; 
        std::shared_ptr<SlidingScreen> workScreen; 

        std::shared_ptr<CursorScreen> cursorScreen; 


    public: 
        CubeDevice(Screen* scrnPtr, Adafruit_ImageReader* reader, Renderable& loading, Renderable& idle, SlidingScreen& work): 
            screenPtr(scrnPtr), 
            loadingScreenComposed(), 
            idleScreenComposed(), 
            workScreenComposed(), 
            loadingScreen(&loading),
            idleScreen(&idle), 
            workScreen(&work),
            cursorScreen(new CursorScreen(*reader)) {

                loadingScreenComposed.addScreen(loadingScreen); 
                idleScreenComposed.addScreen(idleScreen); 
                workScreenComposed.addScreen(workScreen); 

                
                loadingScreenComposed.addScreen(cursorScreen); 
                idleScreenComposed.addScreen(cursorScreen); 
                workScreenComposed.addScreen(cursorScreen); 

                
                // link cursor screen with the cursor state
                cursorScreen->setCursor(&cursorState); 

            
        }

        
        void loop() {
            // rendering
            if (curState == STARTUP) {
                screenPtr->setRenderer(&loadingScreenComposed); 
            } else if (curState == IDLE) {
                screenPtr->setRenderer(&idleScreenComposed); 
            } else if (curState == WORK) {
                screenPtr->setRenderer(&workScreenComposed); 
            }

            
            screenPtr->tryRender(); 
        }



        
        // link handlePacket to packet comm
        void handlePacket(uint8_t type, uint8_t* data, uint8_t len) {
            switch (type) {
                case DEVICE_START:
                    // TODO: transition device out of sleep mode
                    if (curState == STARTUP) curState = IDLE; 
                    break;

                case CURSOR_SET:
                    if (len < 4) return; 
                    cursorState.cursorX = data[0] << 8 | data[1]; 
                    cursorState.cursorY = data[2] << 8 | data[3]; 
                    break;

                case CURSOR_VISIBLE:
                    if (len < 1) return; 
                    cursorState.cursorVisible = data[0] & 0x01; 
                    break; 
                case CURSOR_CLICK:
                    if (cursorState.cursorVisible) screenPtr->getCurrentRenderer()->click(screenPtr->getScreen(), cursorState.cursorX, cursorState.cursorY); 
                    break; 
                case CURSOR_DOWN: 
                    cursorState.mouseDown = true; 
                    break; 
                case CURSOR_UP:
                    cursorState.mouseDown = false; 
                    break; 
                
                
                case SCROLL_LEFT:
                    if (curState == WORK) workScreen->left(); 
                    break;
                case SCROLL_RIGHT: 
                    if (curState == WORK) workScreen->right(); 
                    break; 


                case WEATHER:
                break; 
                case TASKS_ADD:
                break; 
                case CALENDAR_ADD:
                break;
            }
        }

}; 