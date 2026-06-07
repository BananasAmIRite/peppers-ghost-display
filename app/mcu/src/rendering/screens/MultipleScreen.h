#pragma once

#include "../Screen.h"
#include <memory>
#include <vector>

class MultipleScreen : public Renderable {
    private:
        std::vector<std::shared_ptr<Renderable>> screens;

    public:
        MultipleScreen() : screens() {
        }

        MultipleScreen(
            std::vector<std::shared_ptr<Renderable>> screensToRender
        ) : screens(screensToRender) {
        }

        void addScreen(std::shared_ptr<Renderable> scrn) {
            screens.push_back(scrn);
        }

        void render(Adafruit_GFX* display) override {
            for (const auto& screen : screens) {
                screen->render(display); 
            }
        }

        void click(Adafruit_GFX* display, uint16_t x, uint16_t y) override {
            for (const auto& screen : screens) {
                screen->click(display, x, y); 
            }
        }
        
};