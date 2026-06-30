#pragma once

#include "utils/color_utils.h"
#include "utils/sd_utils.h"
#include "utils/ScrollingText.h"
#include "rendering/screens/SpriteScreen.h"
#include "fonts/tiny512pt7b.h"
#include "data/SPIStream.h"
#include "types/packets.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <ArduinoJson.h>


enum TaskStatus {
    NOT_STARTED, 
    IN_PROGRESS, 
    WAITING, 
    DONE
};

struct Task {
    ScrollingText name;
    TaskStatus status;
    std::string id;

    Task(const std::string& text,
         TaskStatus status,
         const std::string& id = "")
        : name(25),      // visible chars
          status(status),
          id(id)
    {
        name.setText(text);
    }
};


class TasksScreen : public Renderable, public SPIStreamHandler {
    private:
        std::vector<Task> tasks; 
    public: 
        TasksScreen();

        void addTask(std::string name, TaskStatus status, std::string id);

        void render(Adafruit_GFX* tft) override;

        
        // handlers
        void onSPIData(uint8_t type, uint32_t size, uint8_t* data) override;
};