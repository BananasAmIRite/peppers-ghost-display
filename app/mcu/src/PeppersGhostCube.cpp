#include "PeppersGhostCube.h"

PeppersGhostCube::PeppersGhostCube(Screen* scrnPtr, Adafruit_ImageReader* reader, UARTComms* packetReceiver, UARTComms* packetReceiver2, SPIStream* spiReceiver) : 
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


void PeppersGhostCube::loop() {
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

void PeppersGhostCube::setScreen(DeviceScreen newScreen) {
    curScreen = newScreen; 
    // if (newScreen == curScreen) return; 
    // nextScreen.next = newScreen; 
    // nextScreen.transitionStart = millis();
    
    // ledcFade(6, 1024, 0, 2000);  
}




// link handlePacket to packet comm
void PeppersGhostCube::onUARTData(uint8_t type, uint8_t* data, uint8_t len) {
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

void PeppersGhostCube::onSPIData(uint8_t type, uint32_t length, uint8_t* body) {

}

