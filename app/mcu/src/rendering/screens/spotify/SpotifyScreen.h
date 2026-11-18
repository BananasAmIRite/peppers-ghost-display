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

// spectrum visualizer
#define SPECTRUM_BANDS 32               // must match NUM_BANDS in windows/SpectrumVisualizer.py
#define SPECTRUM_BAR_WIDTH 7
#define SPECTRUM_BAR_GAP 2
#define SPECTRUM_SEG_HEIGHT 4           // bars are stacks of LED-style segments
#define SPECTRUM_SEG_GAP 1
#define SPECTRUM_SEG_PITCH (SPECTRUM_SEG_HEIGHT + SPECTRUM_SEG_GAP)
#define SPECTRUM_SEGMENTS 34
#define SPECTRUM_MAX_HEIGHT (SPECTRUM_SEGMENTS * SPECTRUM_SEG_PITCH)
#define SPECTRUM_REFLECT_SEGS 4         // faded mirror segments under each bar
#define SPECTRUM_FALL_PER_SEC 4.0f      // bar release speed (fraction of full height per second)
#define SPECTRUM_PEAK_HOLD_MS 300
#define SPECTRUM_PEAK_FALL_PER_SEC 0.8f
#define SPECTRUM_NO_DATA_MS 2000        // show a hint if the laptop hasn't sent anything for this long
#define VIZ_KEEPALIVE_MS 1000
#define SPECTRUM_REFRESH_RATE 60        // target fps on the SPECTRUM page (SPI-bound to ~40 in practice)
#define SPECTRUM_FULL_FRAME_MS 200      // full-screen push this often for the name scroll / slider / overlays

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
    LYRICS,
    SPECTRUM
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

        // spectrum visualizer
        UARTComms* hostLink = nullptr;  // laptop link, told when to start/stop streaming
        bool screenActive = false;      // this screen is the active renderer
        bool vizStreaming = false;      // last state reported to the laptop
        uint32_t lastVizKeepalive = 0;
        uint32_t lastSpectrumPacket = 0;
        uint32_t lastSpectrumFrame = 0;
        float spectrumTarget[SPECTRUM_BANDS] = {0};
        float spectrumLevel[SPECTRUM_BANDS] = {0};
        float spectrumPeak[SPECTRUM_BANDS] = {0};
        uint32_t spectrumPeakTime[SPECTRUM_BANDS] = {0};

        // partial-frame state: only the bar area is pushed to the panel between full frames
        int16_t spectrumRegionX = 0, spectrumRegionY = 0, spectrumRegionW = 0, spectrumRegionH = 0;
        uint32_t lastFullFrame = 0; // 0 forces the next frame to be a full push

        // palette, tinted from the album art's dominant color
        uint16_t segColors[SPECTRUM_SEGMENTS];
        uint16_t reflectColors[SPECTRUM_REFLECT_SEGS];
        uint16_t peakColor;
        uint16_t idlePeakColor;

        void setSpectrumAccent(uint8_t r, uint8_t g, uint8_t b);

        void computeAccentFromArt();

        // streaming should run only while this screen is showing the SPECTRUM page
        void updateVizStreaming();

        void sendVizState(bool active);



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

        void render_spectrum(Adafruit_GFX* tft);

        void setHostLink(UARTComms* link);

        void onActivate() override;

        void onDeactivate() override;

        bool getPartialRegion(int16_t& x, int16_t& y, int16_t& w, int16_t& h) override;

        uint8_t preferredRefreshRate() override;

        std::string seconds_to_str(uint16_t total_seconds);

        uint16_t get_true_elapsed_secs();

        float get_true_elapsed_secs_accurate();


        // handlers
        void onSPIData(uint8_t type, uint32_t size, uint8_t* data) override;

        void onUARTData(uint8_t type, uint8_t* data, uint8_t size, std::string name) override;

        void click(Adafruit_GFX* display, uint16_t x, uint16_t y) override;

};