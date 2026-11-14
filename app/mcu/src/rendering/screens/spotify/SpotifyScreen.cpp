#include "SpotifyScreen.h"

#include "utils/text_utils.h"
#include "utils/color_utils.h"
#include "utils/sd_utils.h"
#include "fonts/tiny512pt7b.h"
#include "fonts/tiny518pt7b.h"
#include "types/packets.h"

#include "ArduinoJson.h"

#include <memory>
#include <utility>
#include <iostream>
#include <iomanip>
#include <sstream>


// TODO: use the modular version
std::string SpotifyScreen::getVisibleName() const {
    const std::string& name = curSong.name;
    if ((int)name.size() <= MAX_SONGNAME_WIDTH)
        return name;
    return name.substr(scrollOffset, MAX_SONGNAME_WIDTH);
}

void SpotifyScreen::updateScroll() {
    const std::string& name = curSong.name;
    if ((int)name.size() <= MAX_SONGNAME_WIDTH) {
        scrollOffset = 0;
        return;
    }

    uint32_t now = millis();
    int maxOffset = (int)name.size() - MAX_SONGNAME_WIDTH;

    if (scrollPaused) {
        if (now - pauseStart >= SCROLL_DELAY_MS) {
            scrollPaused = false;
            lastScrollTime = now;
        }
        return;
    }

    if (now - lastScrollTime < SCROLL_SPEED_MS) return;
    lastScrollTime = now;

    scrollOffset += scrollDir;

    if (scrollOffset >= maxOffset) {
        scrollOffset = maxOffset;
        scrollDir = -1;
        scrollPaused = true;
        pauseStart = now;
    } else if (scrollOffset <= 0) {
        scrollOffset = 0;
        scrollDir = 1;
        scrollPaused = true;
        pauseStart = now;
    }
}

void SpotifyScreen::resetScroll() {
    scrollOffset  = 0;
    scrollDir     = 1;
    scrollPaused  = true;          // pause at start when song changes
    pauseStart    = millis();
    lastScrollTime = millis();
}



SpotifyScreen::SpotifyScreen() : SpriteScreen({"slider0.bmp", "slider1.bmp", "sliderbtn.bmp"}), lyrics(), renderState(IMAGE) {
    setSong("Not Playing...", 100);
    curSong.last_synchronized_millis = millis();
}

void SpotifyScreen::loadBuffer(uint16_t width, uint16_t height, uint8_t* data, uint32_t data_size) {
    curBuffer = std::make_shared<GFXcanvas16>(width, height);
    memcpy(curBuffer->getBuffer(), data, data_size);  
}

void SpotifyScreen::setSong(std::string name, uint16_t length_seconds) {
    curSong.name = name; 
    curSong.length_seconds = length_seconds;
    clearSongLyrics();
    resetScroll();
}

void SpotifyScreen::updateSongState(uint16_t elapsed_seconds, bool paused) {
    curSong.elapsed_seconds = elapsed_seconds;
    curSong.paused = paused; 
    curSong.last_synchronized_millis = millis(); 
}

void SpotifyScreen::clearSongLyrics() {
    lyrics.clear();
}

void SpotifyScreen::addSongLyrics(std::string line, uint16_t progress) {
    lyrics.push_back({line, progress}); 
}

void SpotifyScreen::render(Adafruit_GFX* tft) {
    // updateSongState(millis() / 1000);
    
    updateScroll();


    if (renderState == IMAGE) {
        // song image
        if (curBuffer != nullptr) {
            drawScaledRGBBitmap(*tft, *curBuffer, tft->width() / 2 + SCRN_OFFSET - curBuffer->width() * IMG_SCALE / 2, tft->height() * 3 / 8 - curBuffer->height() * IMG_SCALE / 2, IMG_SCALE); 
        }
    } else if (renderState == LYRICS) {
        render_lyrics(tft);
    } else {
        render_spectrum(tft);
    }

    // keepalive so the laptop stops streaming on its own if we reset / unplug / stop rendering
    if (vizStreaming && millis() - lastVizKeepalive >= VIZ_KEEPALIVE_MS) {
        sendVizState(true);
    }

    // (scrolling) text 
    tft->setFont(&Tiny5_Regular12pt7b);
    tft->setTextColor(COLOR_WHITE);
    tft->setTextSize(1); 

    drawCenteredText(tft, getVisibleName().c_str(), tft->width() / 2 + SCRN_OFFSET, tft->height() * 6 / 8); 


    // slider
    drawSprite(tft, 0, tft->width() / 2 + SCRN_OFFSET - SLIDER_WIDTH * (SCALE + 1) / 2, tft->height() * 7 / 8 - SLIDER_HEIGHT * SCALE / 2, SCALE);
    drawSprite(tft, 1, tft->width() / 2 + SCRN_OFFSET - SLIDER_WIDTH * (SCALE - 1) / 2, tft->height() * 7 / 8 - SLIDER_HEIGHT * SCALE / 2, SCALE);


    // slider head
    int left = tft->width() / 2 + SCRN_OFFSET - SLIDER_WIDTH * (SCALE + 1) / 2; 
    int right = tft->width() / 2 + SCRN_OFFSET - SLIDER_WIDTH * (SCALE - 1) / 2 + SLIDER_WIDTH * SCALE; 
    uint16_t trueElapsedSecs = get_true_elapsed_secs();

    float progress = constrain(((float) trueElapsedSecs) / curSong.length_seconds, 0, 1);
    drawSprite(tft, 2, left + (right - left) * progress - BUTTON_WIDTH * SCALE / 2, tft->height() * 7 / 8 - BUTTON_WIDTH * SCALE / 2, SCALE); 


    // times
    std::string length = seconds_to_str(curSong.length_seconds);
    std::string elapsed = seconds_to_str(trueElapsedSecs);
    
    drawLeftAlignedText(tft, length.c_str(), left + (right - left) * 1 + TIMES_PADDING, tft->height() * 7 / 8); 
    drawRightAlignedText(tft, elapsed.c_str(), left + (right - left) * 0 - TIMES_PADDING, tft->height() * 7 / 8); 
}

void SpotifyScreen::render_lyrics(Adafruit_GFX* tft) {

    if (lyrics.size() == 0) {
        // no lyrics
        drawCenteredText(tft, "No Lyrics...", tft->width() / 2 + SCRN_OFFSET, tft->height() * 3 / 8); 
        return; 
    }

    float elapsed = get_true_elapsed_secs_accurate();
    // find our current lyrics pos
    int lyrics_idx = 0; // NOTE: could be -1, which means we're before the current song
    for (int i = 0; i < lyrics.size(); i++) {
        if (lyrics.at(i).elapsed > elapsed) {
            lyrics_idx = i-1; 
            break; 
        }
    }

    if (elapsed >= lyrics.at(lyrics.size() - 1).elapsed) lyrics_idx = lyrics.size() - 1;

    tft->setFont(&Tiny5_Regular12pt7b); // i == lyrics_idx ? &Tiny5_Regular18pt7b : 
    tft->setTextSize(1); 


    // render my lines
    for (int i = lyrics_idx - 1; i <= lyrics_idx + 1; i++) {
        if (i < 0 || i >= lyrics.size()) continue; // OOB
        
        SongLyricLine curLyrics = lyrics.at(i);
        std::string displayText = curLyrics.line;

        if (i == lyrics_idx) {
            // active lyric

            tft->setTextColor(COLOR_WHITE);

            if (displayText.size() > LYRICS_MAX_CHARS) {
                int maxOffset = displayText.size() - LYRICS_MAX_CHARS;

                int next_lyrics = lyrics_idx+1;
                int line_time = (next_lyrics < lyrics.size() ? (lyrics.at(next_lyrics).elapsed - curLyrics.elapsed) : curSong.length_seconds - curLyrics.elapsed) - 1;
                float cur_line_time = get_true_elapsed_secs_accurate() - curLyrics.elapsed; 
                int scrollOffset = min(((float) cur_line_time / line_time), 1.0f) * maxOffset;   

                displayText =
                    displayText.substr(scrollOffset, LYRICS_MAX_CHARS);
            }
        } else {
            // inactive lyric

            if (displayText.size() > LYRICS_MAX_CHARS) {
                displayText =
                    displayText.substr(0, LYRICS_MAX_CHARS) + "...";
            }
            
            tft->setTextColor(COLOR_GRAY);
        }

        drawCenteredText(
            tft,
            displayText.c_str(),
            tft->width() / 2 + SCRN_OFFSET,
            tft->height() * 3 / 8 + LYRICS_LINE_HEIGHT * (i - lyrics_idx)
        );



        // drawCenteredText(tft, curLyrics.line.c_str(), tft->width() / 2 + SCRN_OFFSET, tft->height() * 3 / 8 + LYRICS_LINE_HEIGHT * (i - lyrics_idx)); 
    }
}

void SpotifyScreen::render_spectrum(Adafruit_GFX* tft) {
    uint32_t now = millis();
    float dt = lastSpectrumFrame == 0 ? 0 : (now - lastSpectrumFrame) / 1000.0f;
    lastSpectrumFrame = now;

    // no recent data from the laptop -> let the bars fall to zero
    bool stale = lastSpectrumPacket == 0 || (now - lastSpectrumPacket) > SPECTRUM_NO_DATA_MS;

    const int totalWidth = SPECTRUM_BANDS * (SPECTRUM_BAR_WIDTH + SPECTRUM_BAR_GAP) - SPECTRUM_BAR_GAP;
    const int left = tft->width() / 2 + SCRN_OFFSET - totalWidth / 2;
    const int bottom = tft->height() * 3 / 8 + SPECTRUM_MAX_HEIGHT / 2;

    for (int i = 0; i < SPECTRUM_BANDS; i++) {
        float target = stale ? 0 : spectrumTarget[i];

        // instant attack, linear release
        if (target >= spectrumLevel[i]) spectrumLevel[i] = target;
        else spectrumLevel[i] = max(target, spectrumLevel[i] - SPECTRUM_FALL_PER_SEC * dt);

        // peak caps: hold briefly, then fall slowly
        if (spectrumLevel[i] >= spectrumPeak[i]) {
            spectrumPeak[i] = spectrumLevel[i];
            spectrumPeakTime[i] = now;
        } else if (now - spectrumPeakTime[i] > SPECTRUM_PEAK_HOLD_MS) {
            spectrumPeak[i] = max(spectrumLevel[i], spectrumPeak[i] - SPECTRUM_PEAK_FALL_PER_SEC * dt);
        }

        int x = left + i * (SPECTRUM_BAR_WIDTH + SPECTRUM_BAR_GAP);
        int h = (int)(spectrumLevel[i] * SPECTRUM_MAX_HEIGHT);
        if (h > 0) tft->fillRect(x, bottom - h, SPECTRUM_BAR_WIDTH, h, COLOR_GREEN);

        int peakY = bottom - (int)(spectrumPeak[i] * SPECTRUM_MAX_HEIGHT) - 3;
        tft->fillRect(x, peakY, SPECTRUM_BAR_WIDTH, 2, COLOR_WHITE);
    }

    if (stale) {
        tft->setFont(&Tiny5_Regular12pt7b);
        tft->setTextSize(1);
        tft->setTextColor(COLOR_GRAY);
        drawCenteredText(tft, "Waiting for audio...", tft->width() / 2 + SCRN_OFFSET, tft->height() * 3 / 8);
    }
}

void SpotifyScreen::setHostLink(UARTComms* link) {
    hostLink = link;
}

void SpotifyScreen::onActivate() {
    SpriteScreen::onActivate();
    screenActive = true;
    updateVizStreaming();
}

void SpotifyScreen::onDeactivate() {
    SpriteScreen::onDeactivate();
    screenActive = false;
    updateVizStreaming();
}

void SpotifyScreen::updateVizStreaming() {
    bool shouldStream = screenActive && renderState == SPECTRUM;
    if (shouldStream == vizStreaming) return;

    vizStreaming = shouldStream;
    if (shouldStream) lastSpectrumFrame = 0; // avoid one huge dt on the first frame
    sendVizState(shouldStream);
}

void SpotifyScreen::sendVizState(bool active) {
    lastVizKeepalive = millis();
    if (hostLink == nullptr) return;
    uint8_t value = active ? 1 : 0;
    hostLink->send(SPOTIFY_VIZ_STATE, &value, 1);
}

std::string SpotifyScreen::seconds_to_str(uint16_t total_seconds) {
    uint16_t hours = total_seconds / 3600;
    uint16_t minutes = (total_seconds % 3600) / 60;
    uint16_t seconds = total_seconds % 60;

    std::ostringstream oss;

    // Set up standard zero-padding formatting
    oss << std::setfill('0');

    if (hours > 0) {
        // Output format: hh:mm:ss
        oss << std::setw(2) << hours << ":"
            << std::setw(2) << minutes << ":"
            << std::setw(2) << seconds;
    } else {
        // Output format: mm:ss
        oss << std::setw(2) << minutes << ":"
            << std::setw(2) << seconds;
    }

    return oss.str();
}

uint16_t SpotifyScreen::get_true_elapsed_secs() {
    return min(curSong.paused ? curSong.elapsed_seconds : (curSong.elapsed_seconds + (millis() - curSong.last_synchronized_millis) / 1000), (unsigned long) curSong.length_seconds);
}

float SpotifyScreen::get_true_elapsed_secs_accurate() {
    return min(curSong.paused ? curSong.elapsed_seconds : (curSong.elapsed_seconds + ((float) millis() - curSong.last_synchronized_millis) / 1000), (float) curSong.length_seconds);
}



// handlers
void SpotifyScreen::onSPIData(uint8_t type, uint32_t size, uint8_t* data) {
    if (type == SPOTIFY_SET_IMAGE) {
        if (size < 4) return; 

        uint16_t width = ((uint16_t)data[1] << 8) | data[0]; 
        uint16_t height = ((uint16_t)data[3] << 8) | data[2];

        LOG("WIDTH: ");
        LOG(width); 
        LOG(", HEIGHT: "); 
        LOGLN(height);

        if (size < 4 + width * height * 2) return; 

        // LOGLN(length); 


        loadBuffer(width, height, data + 4, width * height * 2);


    } else if (type == SPOTIFY_SET_LYRICS) {
        if (size < 2) return; 
        uint16_t jsonSize = ((uint16_t) data[1] << 8) | data[0]; 

        clearSongLyrics(); 
        if (jsonSize == 0) return; 

        std::string jsonString(
            reinterpret_cast<char*>(data + 2),
            jsonSize
        );

        JsonDocument doc;
        deserializeJson(doc, jsonString);
        JsonArray arr = doc.as<JsonArray>();

        for (JsonVariant lyricLine : arr) {
            uint16_t timestamp = lyricLine["timestamp"].as<uint16_t>(); 
            std::string text = lyricLine["text"].as<std::string>();

            addSongLyrics(text, timestamp); 
        }
    }
}

void SpotifyScreen::onUARTData(uint8_t type, uint8_t* data, uint8_t size, std::string name) {
    if (type == SPOTIFY_SET_SONG) {
        if (size < 7) return; 
        uint16_t nameSize;
        uint16_t duration;
        uint16_t progress;
        bool paused = data[6] * 0x01;

        memcpy(&nameSize, data, 2);
        memcpy(&duration, data + 2, 2);
        memcpy(&progress, data + 4, 2);

        std::string songName(
            reinterpret_cast<char*>(data + 7),
            nameSize
        );

        setSong(songName, duration); 
        updateSongState(progress, paused); 
    } else if (type == SPOTIFY_UPDATE_SONG) {
        if (size < 3) return; 
        uint16_t progress;
        bool paused = data[2] * 0x01;
        memcpy(&progress, data, 2);
        updateSongState(progress, paused);
    } else if (type == SPOTIFY_SPECTRUM) {
        if (size < SPECTRUM_BANDS) return;
        for (int i = 0; i < SPECTRUM_BANDS; i++) {
            spectrumTarget[i] = data[i] / 255.0f;
        }
        lastSpectrumPacket = millis();
    }
}

void SpotifyScreen::click(Adafruit_GFX* display, uint16_t x, uint16_t y) {
    // IMAGE -> LYRICS -> SPECTRUM -> IMAGE
    switch (renderState) {
        case IMAGE: renderState = LYRICS; break;
        case LYRICS: renderState = SPECTRUM; break;
        default: renderState = IMAGE; break;
    }
    updateVizStreaming();
}