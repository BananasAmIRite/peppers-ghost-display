#include "CalendarScreen.h"

#include <TimeLib.h>
#include "fonts/tiny512pt7b.h"
#include "fonts/tiny518pt7b.h"
#include "utils/datetime_utils.h"
#include "utils/text_utils.h"
#include "types/packets.h"
#include <ArduinoJson.h>



CalendarScreen::CalendarScreen() {

}

void CalendarScreen::render(Adafruit_GFX* tft) {
    // render date and time

    const int startX = 110; 
    const int startY = 50; 

    const int textMargin = 20; 

    time_t cur_timestamp = computeCurTimestampLocal(); 

    int curMonth = month(cur_timestamp); 
    int curDay = day(cur_timestamp);

    int curHour = hour(cur_timestamp); 
    int curMinute = minute(cur_timestamp);
    int dayOfWeek = weekday(cur_timestamp);
  
    tft->setTextSize(1);
    tft->setTextColor(COLOR_WHITE);
    tft->setFont(&Tiny5_Regular12pt7b);

    tft->setCursor(startX, startY); 

    std::string datetime = dayOfWeekToShortString(dayOfWeek) + ", " + monthToShortString(curMonth) + " " + std::to_string(curDay); 

    int16_t x1, y1; 
    uint16_t w, h; 

    tft->getTextBounds(datetime.c_str(), startX, startY, &x1, &y1, &w, &h); 

    tft->write(datetime.c_str());

    int timeStartX = startX; 
    int timeStartY = startY + h + textMargin + 20; 

    tft->setTextSize(3);
    tft->setTextColor(COLOR_WHITE);
    tft->setFont(&Tiny5_Regular12pt7b);

    tft->setCursor(timeStartX, timeStartY); 

    tft->write(timeTo12hStringNoSeconds(curHour, curMinute).c_str()); 

    
    tft->setTextSize(1);
    tft->setTextColor(COLOR_WHITE);
    tft->setFont(&Tiny5_Regular12pt7b);

    // Draw current and upcoming events

    // first, update all texts in the current and upcoming events
    for (auto& s : currentEvents) s.name.update(); 
    for (auto& s : upcomingEvents) s.name.update(); 


    // then start drawing
    int y = timeStartY + textMargin * 3;

    // ---------- Current events ----------
    if (!currentEvents.empty()) {
        tft->setCursor(startX, y);
        tft->print("Now: ");
        y += textMargin;

        for (size_t i = 0; i < currentEvents.size(); i++) {
            tft->setCursor(startX + 10, y);
            // tft->print("- ");
            tft->print(currentEvents[i].name.getVisibleText().c_str());
            drawRightAlignedText(tft, 
                ("Til " + formatTimeFromTimestamp(timestampToLocal(currentEvents[i].timestampEnd))).c_str(), 
                410, y - textMargin / 2
            ); 
            
            y += textMargin;
        }

        y += textMargin;
    }

    // ---------- Upcoming events ----------
    if (!upcomingEvents.empty()) {
        tft->setCursor(startX, y);
        tft->println("Upcoming:");
        y += textMargin;

        for (const auto& event : upcomingEvents) {
            tft->setCursor(startX + 10, y);
            // tft->print("- ");
            tft->println(event.name.getVisibleText().c_str());
            drawRightAlignedText(tft, 
                (formatTimeFromTimestamp(timestampToLocal(event.timestampStart))).c_str(), 
                410, y - textMargin / 2
            ); 
            y += textMargin;
        }
    }
}

std::vector<RunningCalendarEvent> CalendarScreen::getCurrentEvents() {
    std::vector<RunningCalendarEvent> current;

    time_t now = computeCurTimestamp();

    for (const auto& event : events) {
        if (event.timestampStart <= now && now < event.timestampEnd) {
            current.push_back({event.name, event.timestampStart, event.timestampEnd, 11});
        }
    }

    return current;
}

std::vector<RunningCalendarEvent> CalendarScreen::getNextEvents(size_t count) {
    std::vector<RunningCalendarEvent> upcoming;

    time_t now = computeCurTimestamp();

    // Assumes events are sorted by start time.
    for (const auto& event : events) {
        if (event.timestampStart > now) {
            if ((event.timestampStart - now) > 60 * 60 * 24) continue; 
            upcoming.push_back({event.name, event.timestampStart, event.timestampEnd, 14});

            if (upcoming.size() >= count)
                break;
        }
    }

    return upcoming;
}


void CalendarScreen::onUARTData(uint8_t type, uint8_t* data, uint8_t len, std::string name) {
    if (type == CAL_SET_DATE_TIME && len >= 12) {
        unsigned long long timestamp = 0;
        for (int i = 0; i < 8; i++) {
            timestamp |= static_cast<unsigned long long>(data[i]) << (8 * i);
        }

        int32_t timestamp_offset = 0;
        for (int i = 0; i < 4; i++) {
            timestamp_offset |= static_cast<int32_t>(data[8 + i]) << (8 * i);
        }


        cal_timestamp = timestamp; 
        cal_offset = timestamp_offset;
        last_message = millis(); 
    }
}


void CalendarScreen::onSPIData(uint8_t type, uint32_t size, uint8_t* data) {

    if (type == CAL_SET_EVENTS) {
        if (size < 4) return; 

        uint32_t jsonSize = ((uint32_t)data[3] << 8) | ((uint32_t)data[2] << 8) | ((uint32_t)data[1] << 8) | data[0]; 

        std::string jsonString(
            reinterpret_cast<char*>(data + 4),
            jsonSize
        );

        events.clear(); 

        JsonDocument doc;
        deserializeJson(doc, jsonString);
        JsonArray arr = doc.as<JsonArray>();

        for (JsonVariant event : arr) {
            std::string name = event["name"].as<std::string>(); 
            time_t t_start = event["timestampStart"].as<time_t>();
            time_t t_end = event["timestampEnd"].as<time_t>();

            events.push_back({name, t_start, t_end}); 
        }

        // update current and upcoming events
        currentEvents = getCurrentEvents(); 
        upcomingEvents = getNextEvents(4 - currentEvents.size()); 
    }

}