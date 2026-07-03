#include "PeppersGhostCube.h"

PeppersGhostCube::PeppersGhostCube(Screen* scrnPtr, Adafruit_ImageReader* reader, UARTComms* packetReceiver, UARTComms* packetReceiver2, SPIStream* spiReceiver) : 
    screenPtr(scrnPtr), 
    loadingScreen(333), 
    idleScreen(), 
    weatherScreen(), 
    tasksScreen(), 
    spotifyScreen(), 
    calScreen(),
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

        packetReceiver->addUARTHandler(&calScreen); 
        packetReceiver2->addUARTHandler(&calScreen);
        spiStreamPtr->addHandler(&calScreen); 

        screenPtr->setAuxRenderer(&cursorScreen);
}


void PeppersGhostCube::loop() {

    // check heartbeat and set state as necessary
    if ((millis() - lastHeartbeat) > 5000) curScreen = STARTUP;


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
        case CALENDAR:
            screenPtr->setRenderer(&calScreen);
            break;
        case SPOTIFY: 
            screenPtr->setRenderer(&spotifyScreen); 
            break;
    }



    if (nextScreen.phase == TransitionPhase::WAITING_FOR_LOAD) {
        screenReadyAfterLoad = true;
    }
    
    screenPtr->tryRender(); 
    
}

void PeppersGhostCube::updatePWMState() {
    switch (nextScreen.phase) {

        case TransitionPhase::FADING_OUT: {
            long timePassed = millis() - nextScreen.transitionStart;
            long half = SCREEN_TRANSITION_TIME / 2;

            if (timePassed >= half) {
                // fully black — switch the actual screen now, then wait
                curScreen = nextScreen.next;
                screenPtr->setPWMValue(0);
                nextScreen.phase = TransitionPhase::WAITING_FOR_LOAD;
            } else {
                // fade 1023 -> 0 over [0, half]
                float t = (float)timePassed / half;               // 0..1
                float val = 1023 * (0.5f * (cos(PI * t) + 1));      // eases 1023 -> 0
                screenPtr->setPWMValue((int)val);
            }
            break;
        }

        case TransitionPhase::WAITING_FOR_LOAD: {
            screenPtr->setPWMValue(0);   // stay black
            if (screenReadyAfterLoad) {
                nextScreen.transitionStart = millis();
                nextScreen.phase = TransitionPhase::HOLD_BLACK;   // extra buffer before fading in
            }
            break;
        }

        case TransitionPhase::HOLD_BLACK: {
            screenPtr->setPWMValue(0);
            long timePassed = millis() - nextScreen.transitionStart;
            if (timePassed >= SCREEN_LOAD_BUFFER_MS) {   // e.g. 150-300ms, tune to taste
                nextScreen.transitionStart = millis();
                nextScreen.phase = TransitionPhase::FADING_IN;
            }
            break;
        }

        case TransitionPhase::FADING_IN: {
            long timePassed = millis() - nextScreen.transitionStart;
            long half = SCREEN_TRANSITION_TIME / 2;

            if (timePassed >= half) {
                screenPtr->setPWMValue(1023);
                nextScreen.phase = TransitionPhase::NONE;
                nextScreen.next = DeviceScreen::NONE;
            } else {
                float t = (float)timePassed / half;
                float val = 1023 * (0.5f * (1 - cos(PI * t)));      // eases 0 -> 1023
                screenPtr->setPWMValue((int)val);
            }
            break;
        }

        case TransitionPhase::NONE:
        default:
            screenPtr->setPWMValue(curScreen == OFF ? 0 : 1023);
            break;
    }
}

void PeppersGhostCube::setScreen(DeviceScreen newScreen) {
    // curScreen = newScreen; 
    if (newScreen == curScreen) return; 
    nextScreen.next = newScreen; 
    nextScreen.transitionStart = millis();
    nextScreen.phase = TransitionPhase::FADING_OUT;
    screenReadyAfterLoad = false;
    // ledcFade(6, 1024, 0, 2000);  
}




// link handlePacket to packet comm
void PeppersGhostCube::onUARTData(uint8_t type, uint8_t* data, uint8_t len, std::string name) {
    switch (type) {

        case PI_SWIPE: {
            if (len < 1) return; 
            // TODO: make this an actual pattern so we don't have to write it each time
            Serial1.write(0x55);
            Serial1.write(0x55);
            Serial1.write(0x02); // swipe type + direction
            Serial1.write(PI_SWIPE);
            Serial1.write(data[0]); // swipe direction


            break; 
        }

        case SET_SCREEN: 
            if (len < 1) return;
            setScreen((DeviceScreen) data[0]); 
            break;

        case CURSOR_CLICK:
            if (cursorScreen.getCursor()->cursorVisible) screenPtr->getCurrentRenderer()->click(screenPtr->getScreen(), cursorScreen.getCursor()->cursorX, cursorScreen.getCursor()->cursorY); 
            break; 

        case DEBUG_HEARTBEAT: {
            lastHeartbeat = millis(); 
            break; 
        }

        default:
            break; 
    }

    // testing

    // Serial1.write(0x32);
    // Serial1.write(0xFF);
}

void PeppersGhostCube::onSPIData(uint8_t type, uint32_t length, uint8_t* body) {

}

Screen* PeppersGhostCube::getScreen() {
    return screenPtr; 
}