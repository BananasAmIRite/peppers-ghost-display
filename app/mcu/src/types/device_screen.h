#pragma once

#include <string>

enum DeviceScreen {
    STARTUP,
    // perm 
    IDLE, 
    WEATHER, 
    TASKS,

    // temp
    SPOTIFY, 
    MEETINGS, 
    NONE
};

inline std::string screen_to_text(DeviceScreen screen) {
    switch (screen) {
        case STARTUP: 
            return "Startup"; 
        case IDLE:
            return "Idle"; 
        case WEATHER: 
            return "Weather"; 
        case TASKS: 
            return "Tasks"; 
        case SPOTIFY:
            return "Spotify";
        case MEETINGS:
            return "Meetings";
        default:
            return "";
    };
}