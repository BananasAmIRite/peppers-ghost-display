#pragma once

#include "Arduino.h"
// #include <Adafruit_ST7789.h>
#include <Adafruit_HX8357.h>
#include <Adafruit_GFX.h>
#include "utils/color_utils.h"

class Renderable {
public:
    virtual void render(Adafruit_GFX* display) = 0;
    virtual void click(Adafruit_GFX* display, uint16_t x, uint16_t y) {}
    virtual void onActivate() {}    // called when this becomes the active renderer
    virtual void onDeactivate() {}  // called when it stops being the active renderer
};

class EmptyScreen : public Renderable {
    public:
        void render(Adafruit_GFX* display) {}
        virtual void click(Adafruit_GFX* display, uint16_t x, uint16_t y) {}
};

class Screen {
    private:  
        Adafruit_HX8357* tft;
        uint8_t refreshRate; 
        long lastRenderTime = -1;
        Renderable* toBeRendered = nullptr; 
        Renderable* auxRenderable = nullptr; 
        GFXcanvas16* buffer = nullptr;

        // screen pwm
        int pwm_pin; 

    public:

        Screen(Adafruit_HX8357* tft_hndl, uint8_t screen_refresh_rate, int pwm_pin): tft(tft_hndl), refreshRate(screen_refresh_rate), pwm_pin(pwm_pin) {
            buffer = new GFXcanvas16(tft_hndl->width(), tft_hndl->height());
            buffer->setRotation(1);
            pinMode(pwm_pin, OUTPUT); 
              // Set frequency to 5000 Hz (5 kHz)
            analogWriteFrequency(pwm_pin, 5000);
            analogWriteResolution(pwm_pin, 10); 
        }
        void tryRender();
        void update();

        Adafruit_GFX* getScreen() {
            return buffer;
        }

        Renderable* getCurrentRenderer() {
            return toBeRendered;
        }

        void setRenderer(Renderable* renderer) {
            if (toBeRendered == renderer) return;  // no change, do nothing
            if (toBeRendered != nullptr) toBeRendered->onDeactivate();
            if (renderer != nullptr) renderer->onActivate();
            toBeRendered = renderer;
        }

        void setAuxRenderer(Renderable* renderer) {
            if (auxRenderable == renderer) return;  // no change, do nothing
            if (auxRenderable != nullptr) auxRenderable->onDeactivate();
            if (renderer != nullptr) renderer->onActivate();
            auxRenderable = renderer;
        }

        void clearScreen() {
            buffer->fillScreen(COLOR_BLACK);
        }

        void setPWMOutput(int value) {
            if (pwm_pin != -1) {
                analogWrite(pwm_pin, value); 
            }
        }
};