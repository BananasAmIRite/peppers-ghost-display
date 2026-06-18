#pragma once

enum PacketType {
    CURSOR_SET = 0x10, 
    CURSOR_VISIBLE = 0x11, 
    CURSOR_CLICK = 0x12, 
    CURSOR_DOWN = 0x13, 
    CURSOR_UP = 0x14,
    
    SET_SCREEN = 0x20, 
    SET_SCREEN_NOTIF = 0x21, 

    WEATHER_SET = 0x30, 
    TASKS_ADD = 0x31, 
    CALENDAR_ADD = 0x32, 

    // SPOTIFY
    SPOTIFY_SET_SONG = 0x40, 
    SPOTIFY_SET_IMAGE = 0x41,


    // debug
    DEBUG_SET_IMAGE = 0xF0
};