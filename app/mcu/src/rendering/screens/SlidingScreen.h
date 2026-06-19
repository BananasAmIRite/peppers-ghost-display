#pragma once

#include "utils/color_utils.h"
#include "rendering/Screen.h"
#include <vector>
#include <memory>

#define CIRCLES_PAD_BOTTOM 10
#define CIRCLES_PAD_SPACING 20
#define CIRCLES_RADIUS 5
#define CIRCLE_COLOR COLOR_WHITE

class SlidingScreen : public Renderable {
    private:
        std::vector<std::shared_ptr<Renderable>> screens; 
        size_t curIdx = 0; 
        bool displayDots; 
    public: 
        SlidingScreen(bool displayCircles = true): screens(), displayDots(displayCircles) {}

        SlidingScreen(std::vector<std::shared_ptr<Renderable>> screensToRender, bool displayCircles = true): screens(screensToRender), displayDots(displayCircles) {
            
        }

        void render(Adafruit_GFX* tft) override {
            // render the current screen
            if (curIdx >= 0 && curIdx < screens.size()) {
                screens[curIdx]->render(tft);
            }

            if (!displayDots) return; 

            // draw dots at the bottom
            int16_t width = tft->width(); 
            int16_t height = tft->height();
            
            for (size_t i = 0; i < screens.size(); i++) {
                int16_t circleX = width / 2 + (i - (((int16_t) screens.size()) - 1) / 2.0) * CIRCLES_PAD_SPACING; 
                int16_t circleY = height - CIRCLES_PAD_BOTTOM; 
                tft->drawCircle(circleX, circleY, CIRCLES_RADIUS, CIRCLE_COLOR); 
                if (curIdx == i) {
                    tft->fillCircle(circleX, circleY, CIRCLES_RADIUS, CIRCLE_COLOR);
                }
            }
        }

        void left() {
            curIdx = (curIdx - 1) % screens.size(); 
        }

        void right() {
            curIdx = (curIdx + 1) % screens.size(); 
        }

        void setIndex(size_t idx) {
            curIdx = idx; 
        }

        void addScreen(std::shared_ptr<Renderable> screen) {
            screens.push_back(screen); 
        } 

        void onActivate() override {
            for (const auto& screen : screens) screen->onActivate();
        }
        void onDeactivate() override {
            for (const auto& screen : screens) screen->onDeactivate();
        }
};