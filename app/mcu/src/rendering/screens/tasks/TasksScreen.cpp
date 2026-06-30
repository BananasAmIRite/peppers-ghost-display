#include "TasksScreen.h"


TasksScreen::TasksScreen() : Renderable(), tasks() {
}

void TasksScreen::addTask(std::string name, TaskStatus status, std::string id) {
    tasks.push_back({name, status, id});
}

void TasksScreen::render(Adafruit_GFX* tft) {
    const int marginX = 100;
    const int startY = 40;

    const int circleRadius = 4;
    const int circleX = marginX + circleRadius;

    const int textX = circleX + circleRadius + 6;
    const int lineHeight = 25;

    tft->setTextSize(1);
    tft->setTextColor(COLOR_WHITE);
    tft->setFont(&Tiny5_Regular12pt7b);

    for (size_t i = 0; i < tasks.size(); i++) {
        Task& task = tasks[i];

        task.name.update();

        int y = startY + i * lineHeight;

        uint16_t color;
        switch (task.status) {
            case NOT_STARTED: color = COLOR_GRAY;  break;
            case IN_PROGRESS: color = COLOR_BLUE;  break;
            case WAITING:     color = COLOR_RED;   break;
            case DONE:        color = COLOR_GREEN; break;
            default:          color = COLOR_WHITE; break;
        }

        tft->fillCircle(circleX, y, circleRadius, color);

        tft->setCursor(textX, y + 3);
        tft->print(task.name.getVisibleText().c_str());
    }
}



void TasksScreen::onSPIData(uint8_t type, uint32_t size, uint8_t* data) {
    if (type == TASKS_ADD) {
        if (size < 4) return; 

        uint32_t jsonSize = ((uint32_t)data[3] << 8) | ((uint32_t)data[2] << 8) | ((uint32_t)data[1] << 8) | data[0]; 

        std::string jsonString(
            reinterpret_cast<char*>(data + 4),
            jsonSize
        );

        JsonDocument doc;
        deserializeJson(doc, jsonString);
        JsonArray arr = doc.as<JsonArray>();

        for (JsonVariant task : arr) {
            std::string action = task["action"].as<std::string>(); 
            std::string id = task["id"].as<std::string>();
            if (action == "UPDATE") {

                TaskStatus status = (TaskStatus) task["status"].as<int>();
                std::string name = task["name"].as<std::string>();

                size_t idx = -1;
                for (size_t i = 0; i < tasks.size(); i++) if (tasks.at(i).id == id) {idx = i; break; } 

                if (idx == -1) {
                    // no idx found, add a new task
                    addTask(name, status, id);
                } else {
                    tasks.at(idx).name.setText(name);
                    tasks.at(idx).status = status;
                }
            } else if (action == "REMOVE") {
                size_t idx = -1;
                for (size_t i = 0; i < tasks.size(); i++) if (tasks.at(i).id == id) {idx = i; break; } 

                if (idx != -1) tasks.erase(tasks.begin() + idx); 
            }
        }


    }
}