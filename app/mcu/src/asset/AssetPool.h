#pragma once

#include <Adafruit_ImageReader.h>
#include <map>
#include <string>
#include "../Debug.h"

class AssetPool {
public:
 
    static void init(Adafruit_ImageReader& reader) {
        instance()._reader = &reader;
    }
 
    static AssetPool& instance() {
        static AssetPool pool;
        return pool;
    }
 
    /**
     * Acquire an image by file path.
     * Loads from SD on first acquire; returns the cached copy on subsequent
     * calls. Returns nullptr if the load failed.
     * Increments the reference count regardless of load success so that
     * every acquire() must be paired with a release().
     */
    Adafruit_Image* acquire(const std::string& path) {
        auto it = _entries.find(path);
 
        if (it != _entries.end()) {
            it->second.refCount++;
            return it->second.image;
        }
 
        // First time: load from SD.
        Entry entry;
        entry.image = new Adafruit_Image();
        entry.refCount = 1;
 
        if (_reader == nullptr) {
            LOGLN("[AssetPool] ERROR: not initialised — call AssetPool::init() after SD.begin()");
            // Still insert so release() can clean up without crashing.
            _entries[path] = entry;
            return nullptr;
        }
 
        ImageReturnCode rc = _reader->loadBMP(path.c_str(), *entry.image);
        LOG("[AssetPool] load ");
        LOG(path.c_str());
        LOG(" -> ");
        LOGLN(rc);
 
        if (rc != IMAGE_SUCCESS) {
            delete entry.image;
            entry.image = nullptr;
            LOGLN("Load unsuccessful...");
        }
 
        _entries[path] = entry;
        return entry.image;
    }
 
    /**
     * Release a previously acquired path.
     * When the reference count reaches zero the Adafruit_Image is deleted and
     * the entry is removed from the map, freeing heap memory immediately.
     */
    void release(const std::string& path) {
        auto it = _entries.find(path);
        if (it == _entries.end()) {
            LOG("[AssetPool] WARNING: release() called for untracked path: ");
            LOGLN(path.c_str());
            return;
        }
 
        Entry& entry = it->second;
        if (entry.refCount > 0) entry.refCount--;
 
        if (entry.refCount == 0) {
            delete entry.image;
            _entries.erase(it);
            LOG("[AssetPool] freed ");
            LOGLN(path.c_str());
        }
    }
 
    /** Diagnostic: print all live entries and their ref counts. */
    void printStats() const {
        LOGLN("[AssetPool] --- live assets ---");
        for (const auto& kv : _entries) {
            LOG("  ");
            LOG(kv.first.c_str());
            LOG("  refs=");
            LOG(kv.second.refCount);
            LOG("  ptr=");
            LOGLN(kv.second.image != nullptr ? "ok" : "null");
        }
        LOGLN("[AssetPool] ---");
    }
 
private:
    struct Entry {
        Adafruit_Image* image = nullptr;
        uint8_t refCount = 0;
    };
 
    std::map<std::string, Entry> _entries;
    Adafruit_ImageReader* _reader = nullptr;
 
    // Singleton: no public construction.
    AssetPool() = default;
    AssetPool(const AssetPool&) = delete;
    AssetPool& operator=(const AssetPool&) = delete;
};