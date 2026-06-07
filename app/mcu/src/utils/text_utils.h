#pragma once
#include <Adafruit_GFX.h>

void drawCenteredText(Adafruit_GFX* display,
                      const char* text,
                      int16_t centerX,
                      int16_t centerY)
{
    int16_t x1, y1;
    uint16_t w, h;

    display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    int16_t x = centerX - w / 2;
    int16_t y = centerY + h / 2;

    display->setCursor(x, y);
    display->print(text);
}