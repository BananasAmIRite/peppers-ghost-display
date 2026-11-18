#include "Screen.h"
#include <math.h>

void Screen::tryRender() {
    uint8_t rate = refreshRate;
    bool fastRate = toBeRendered != NULL && toBeRendered->preferredRefreshRate() > rate;
    if (fastRate) rate = toBeRendered->preferredRefreshRate();

    if (lastRenderTime == -1 || (millis() - lastRenderTime) > 1000.0 / rate) {
        // renderers asking for a higher rate get a true frame period (stamped before render + SPI);
        // everything else keeps the original behavior of idling a full period after each frame
        if (fastRate) lastRenderTime = millis();

        if (toBeRendered != NULL) {
            clearScreen();
            toBeRendered->render(buffer);
            auxRenderable->render(buffer);

            int16_t x, y, w, h;
            bool partial = toBeRendered->getPartialRegion(x, y, w, h) &&
                           (auxRenderable == nullptr || auxRenderable->allowPartialFrames());
            if (partial) updateRegion(x, y, w, h);
            else update();
        }

        if (!fastRate) lastRenderTime = millis();
    }
}

void Screen::update() {
    tft->drawRGBBitmap(0, 0, buffer->getBuffer(), tft->width(), tft->height());
}

// Push one rectangle of the framebuffer to the panel. (x, y, w, h) are in the canvas's rotated
// (logical, 480x320) space; the buffer itself is stored unrotated (320x480), which is also the
// panel's address space, so the rect is mapped back before sending.
void Screen::updateRegion(int16_t x, int16_t y, int16_t w, int16_t h) {
    // clip to the logical screen
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > buffer->width()) w = buffer->width() - x;
    if (y + h > buffer->height()) h = buffer->height() - y;
    if (w <= 0 || h <= 0) return;

    // rotation 1: logical (lx, ly) -> raw (RAW_W - 1 - ly, lx)
    const int16_t rawStride = tft->width();
    int16_t rawX = rawStride - (y + h);
    int16_t rawY = x;
    int16_t rawW = h;
    int16_t rawH = w;

    uint16_t* row = buffer->getBuffer() + rawY * rawStride + rawX;
    tft->startWrite();
    tft->setAddrWindow(rawX, rawY, rawW, rawH);
    for (int16_t r = 0; r < rawH; r++) {
        tft->writePixels(row, rawW);
        row += rawStride;
    }
    tft->endWrite();
}
