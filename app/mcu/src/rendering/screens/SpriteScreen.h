#pragma once

#include "../../utils/color_utils.h"
#include "../../utils/sd_utils.h"
#include "../Screen.h"
#include <Adafruit_ImageReader.h>
#include <memory>
#include <vector>
#include <string>
#include <utility>

class SpriteScreen : public Renderable {
    private:
        std::vector<std::string> spriteFiles;
        std::vector<std::unique_ptr<Adafruit_Image>> preloadedSprites;
        Adafruit_ImageReader* imgReader = nullptr; 
        bool preloadAtInstantiation;
        bool hasPreloaded = false; 

        void preloadSprites() {
            preloadedSprites.clear();
            preloadedSprites.reserve(spriteFiles.size());

            for (const auto& spriteFile : spriteFiles) {
                std::unique_ptr<Adafruit_Image> image(new Adafruit_Image());
                ImageReturnCode val = imgReader->loadBMP(spriteFile.c_str(), *image); 
                Serial.print("Loading image: ");
                Serial.print(spriteFile.c_str());  
                Serial.print("; Result: "); 
                Serial.println(val); 
                if (imgReader != nullptr && val == IMAGE_SUCCESS) {
                    preloadedSprites.push_back(std::move(image));
                } else {
                    preloadedSprites.push_back(nullptr);
                }
            }
            
                Serial.print("Preloaded sprites: "); 
                Serial.println(preloadedSprites.size()); 
        }

    protected: 
        
        void drawSprite(Adafruit_GFX* tft, size_t spriteIndex, int16_t x, int16_t y, uint8_t scale) {
            // Serial.println("Drawing current sprite..."); 
            // Serial.print(spriteIndex);
            // Serial.print(", ");
            // Serial.print(x);
            // Serial.print(", ");
            // Serial.print(y);
            // Serial.print(", ");
            // Serial.println(scale);
            if (spriteFiles.empty()) {
                return;
            }

            if (preloadAtInstantiation && spriteIndex < preloadedSprites.size() && preloadedSprites[spriteIndex] != nullptr) {
                // Serial.println("Drawing loaded sprite..."); 
                drawLoadedBMPToGFX(*preloadedSprites[spriteIndex], *tft, x, y, scale);
                return;
            }

            if (imgReader != nullptr) {
                // Serial.println("Drawing and loading sprite..."); 
                // TODO: impl scale
                drawBMPToGFX(*imgReader, spriteFiles[spriteIndex].c_str(), *tft, x, y);
            }
        }
    public: 

        SpriteScreen(
            Adafruit_ImageReader& reader,
            std::vector<std::string> filePaths,
            bool preloadSpritesOnInstantiation = false
        ) : spriteFiles(filePaths), imgReader(&reader), preloadAtInstantiation(preloadSpritesOnInstantiation) {
            
        }

        void render(Adafruit_GFX* tft) override {
            if (preloadAtInstantiation && !hasPreloaded) {
                preloadSprites();
                hasPreloaded = true; 
            }
        }
};