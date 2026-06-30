#pragma once

#include "rendering/screens/SpriteScreen.h"
#include <vector>
#include <string>
#include "Adafruit_GFX.h"
#include "data/SPIStream.h"
#include "data/UARTComms.h"

#define SLIDER_WIDTH 32
#define SLIDER_HEIGHT 3
#define SCALE 4
#define BUTTON_WIDTH 5

#define IMG_SCALE 3

#define TIMES_PADDING 10

#define MAX_SONGNAME_WIDTH 25
#define SCROLL_DELAY_MS 600   // pause at each end before scrolling back
#define SCROLL_SPEED_MS 120   // ms per character step

#define LYRICS_LINE_HEIGHT 40
#define LYRICS_MAX_CHARS 25

struct SongState {
    std::string name; 
    uint16_t length_seconds; 
    uint16_t elapsed_seconds; 
    long last_synchronized_millis; 
    bool paused; 
};

struct SongLyricLine {
    std::string line; 
    uint16_t elapsed; 
};

enum SpotifyRenderState {
    IMAGE, 
    LYRICS
};

class SpotifyScreen : public SpriteScreen, public SPIStreamHandler, public UARTHandler {
    private:

        std::shared_ptr<GFXcanvas16> curBuffer; 

        SongState curSong; 

        // scroll state
        int scrollOffset = 0;        // current char offset into the name
        int scrollDir    = 1;        // +1 = forward, -1 = backward
        uint32_t lastScrollTime = 0;
        bool scrollPaused = false;
        uint32_t pauseStart = 0;

        std::vector<SongLyricLine> lyrics;

        SpotifyRenderState renderState; 



        // returns the visible slice of the song name based on current offset
        std::string getVisibleName() const;

        void updateScroll();

        void resetScroll();

    public: 
        SpotifyScreen();

        void loadBuffer(uint16_t width, uint16_t height, uint8_t* data, uint32_t data_size);

        void setSong(std::string name, uint16_t length_seconds);

        void updateSongState(uint16_t elapsed_seconds, bool paused);

        void clearSongLyrics();

        void addSongLyrics(std::string line, uint16_t progress);

        void render(Adafruit_GFX* tft) override;

        void render_lyrics(Adafruit_GFX* tft);

        std::string seconds_to_str(uint16_t total_seconds);

        uint16_t get_true_elapsed_secs();

        float get_true_elapsed_secs_accurate();


        // handlers
        void onSPIData(uint8_t type, uint32_t size, uint8_t* data) override;

        void onUARTData(uint8_t type, uint8_t* data, uint8_t size) override;

        void click(Adafruit_GFX* display, uint16_t x, uint16_t y) override;

};