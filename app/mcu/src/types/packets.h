#pragma once

enum PacketType {
    CURSOR_SET = 0x10, 
    CURSOR_VISIBLE = 0x11, 
    CURSOR_CLICK = 0x12, 
    CURSOR_DOWN = 0x13, 
    CURSOR_UP = 0x14,
    
    SET_SCREEN = 0x21, 
    SET_SCREEN_NOTIF = 0x22, 

    // WEATHER (uart)
    WEATHER_SET = 0x30, 


    // TASKS (spi)
    /*
    Tasks Json Format: 
    [{
        "action": "UPDATE",
        "name": "...", 
        "status": 0 | 1 | 2, 
        "id": "..."
    }, 
    {
    "action": "REMOVE", 
    "id": "..."
    }]
    
    */
    TASKS_ADD = 0x31,

    // SPOTIFY
    SPOTIFY_SET_SONG = 0x40, // uart
    SPOTIFY_SET_IMAGE = 0x41, // spi
    SPOTIFY_UPDATE_SONG = 0x42, // uart
    SPOTIFY_SET_LYRICS = 0x43, // spi


    // debug
    DEBUG_SET_IMAGE = 0xF0
};