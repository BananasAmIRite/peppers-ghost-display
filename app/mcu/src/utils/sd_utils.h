#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ImageReader.h>

constexpr uint16_t TRANSPARENT = 0xF81F; // bright magenta

inline void drawScaledRGBBitmap(
    Adafruit_GFX& dest,
    GFXcanvas16& src,
    int16_t x,
    int16_t y,
    uint8_t scale)
{
    uint16_t* srcBuf = src.getBuffer();

    int16_t w = src.width();
    int16_t h = src.height();

    for (int16_t sy = 0; sy < h; sy++) {
        for (int16_t sx = 0; sx < w; sx++) {

            uint16_t color = srcBuf[sy * w + sx];

            if (color == TRANSPARENT) continue; 

            dest.fillRect(
                x + sx * scale,
                y + sy * scale,
                scale,
                scale,
                color);
        }
    }
}

inline ImageReturnCode drawLoadedBMPToGFX(Adafruit_Image& image, Adafruit_GFX& canvas, int16_t x, int16_t y, uint8_t scale = 1) {
    if (image.getFormat() != IMAGE_16) {
        return IMAGE_ERR_FORMAT;
    }

    // if (scale != 1) {
    GFXcanvas16* src = (GFXcanvas16*)image.getCanvas();
    drawScaledRGBBitmap(canvas, *src, x, y, scale);
    // } else {
    //     canvas.drawRGBBitmap(x, y, ((GFXcanvas16*) image.getCanvas())->getBuffer(), image.width(), image.height()); 
    // }
    return IMAGE_SUCCESS;
}

inline ImageReturnCode drawBMPToGFX(Adafruit_ImageReader& reader, const char* fileName, Adafruit_GFX& canvas, int16_t x, int16_t y) {
    Adafruit_Image image; 
    ImageReturnCode status = reader.loadBMP(fileName, image); 
    if (status != IMAGE_SUCCESS) {
        return status;
    }

    return drawLoadedBMPToGFX(image, canvas, x, y); 

}
