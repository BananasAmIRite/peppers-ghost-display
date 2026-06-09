#pragma once

#include "../../utils/color_utils.h"
#include "../../utils/sd_utils.h"
#include "../Screen.h"
#include "../../asset/AssetPool.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>
class SpriteScreen : public Renderable {
private:
    std::vector<std::string> _spriteFiles;
 
    // Non-owning view into AssetPool — the pool owns the Adafruit_Image.
    // Indexed 1:1 with _spriteFiles.
    std::vector<Adafruit_Image*> _sprites;
 
    bool _loaded = false;
 
protected:
 
    void drawSprite(Adafruit_GFX* tft, size_t spriteIndex, int16_t x, int16_t y, uint8_t scale) {
        if (spriteIndex >= _sprites.size() || _sprites[spriteIndex] == nullptr) {
            LOG("WARNING: sprite OOB; Sprite index: ");
            LOG(spriteIndex);
            LOG(", Sprite Size: "); 
            LOGLN(_sprites.size()); 
            return;
        }
        drawLoadedBMPToGFX(*_sprites[spriteIndex], *tft, x, y, scale);
    }
 
public:
 
    SpriteScreen(std::vector<std::string> filePaths)
        : _spriteFiles(std::move(filePaths))
    {}
 
    /**
     * Acquire all sprites from the pool.
     * Safe to call multiple times — skips if already loaded.
     */
    void preload() {
        if (_loaded) return;
 
        _sprites.clear();
        _sprites.reserve(_spriteFiles.size());
 
        for (const auto& path : _spriteFiles) {
            _sprites.push_back(AssetPool::instance().acquire(path));
        }
 
        _loaded = true;
    }
 
    /**
     * Release all sprites back to the pool.
     * The pool will free the underlying image if no other screen holds it.
     * Safe to call if not loaded.
     */
    void freeSprites() {
        if (!_loaded) return;
 
        for (const auto& path : _spriteFiles) {
            AssetPool::instance().release(path);
        }
 
        _sprites.clear();
        _loaded = false;
    }
 
    bool isLoaded() const { return _loaded; }
 
    // Subclasses must call SpriteScreen::render() first as a safety net.
    // Controlled preloading via preload() is always preferred — hitting this
    // path means a state transition forgot to call preload(), and will cause
    // a visible frame hitch while SD loads.
    void render(Adafruit_GFX* tft) override {
        if (!_loaded) {
            LOGLN("[SpriteScreen] WARNING: render() called before preload() -- loading now");
            preload();
        }
    }

    void onActivate() override { preload(); }
    void onDeactivate() override { freeSprites(); }
};