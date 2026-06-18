#include "Screen.h"
#include <math.h>

void Screen::tryRender() {
    if (lastRenderTime == -1 || (millis() - lastRenderTime) > 1000.0 / refreshRate) {
        if (toBeRendered != NULL) {
            clearScreen();
            toBeRendered->render(buffer);
            auxRenderable->render(buffer);
            update(); 
        }

        lastRenderTime = millis();
    }
}

void Screen::update() {
    tft->drawRGBBitmap(0, 0, buffer->getBuffer(), tft->width(), tft->height());
}

