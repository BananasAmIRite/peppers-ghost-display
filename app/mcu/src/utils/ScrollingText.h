#pragma once

#include <string>
#include <Arduino.h>

class ScrollingText {
private:
    std::string text;

    int maxVisibleChars;

    int scrollOffset = 0;
    int scrollDir = 1;

    uint32_t lastScrollTime = 0;
    bool scrollPaused = true;
    uint32_t pauseStart = 0;

    uint32_t scrollDelayMs;
    uint32_t scrollSpeedMs;

public:
    ScrollingText(
        int maxVisibleChars,
        uint32_t scrollDelayMs = 600,
        uint32_t scrollSpeedMs = 120
    )
        : maxVisibleChars(maxVisibleChars),
          scrollDelayMs(scrollDelayMs),
          scrollSpeedMs(scrollSpeedMs) {}

    void setText(const std::string& newText) {
        text = newText;
        reset();
    }

    void reset() {
        scrollOffset = 0;
        scrollDir = 1;

        scrollPaused = true;
        pauseStart = millis();
        lastScrollTime = millis();
    }

    void update() {
        if ((int)text.size() <= maxVisibleChars) {
            scrollOffset = 0;
            return;
        }

        uint32_t now = millis();

        int maxOffset = text.size() - maxVisibleChars;

        if (scrollPaused) {
            if (now - pauseStart >= scrollDelayMs) {
                scrollPaused = false;
                lastScrollTime = now;
            }
            return;
        }

        if (now - lastScrollTime < scrollSpeedMs) {
            return;
        }

        lastScrollTime = now;

        scrollOffset += scrollDir;

        if (scrollOffset >= maxOffset) {
            scrollOffset = maxOffset;
            scrollDir = -1;

            scrollPaused = true;
            pauseStart = now;
        }
        else if (scrollOffset <= 0) {
            scrollOffset = 0;
            scrollDir = 1;

            scrollPaused = true;
            pauseStart = now;
        }
    }

    std::string getVisibleText() const {
        if ((int)text.size() <= maxVisibleChars) {
            return text;
        }

        return text.substr(scrollOffset, maxVisibleChars);
    }

    const std::string& getFullText() const {
        return text;
    }

    bool isScrolling() const {
        return text.size() > maxVisibleChars;
    }
};