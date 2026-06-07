# HOLOGRAPHIC DISPLAY

## Initial Features List:

- Automatic BLE connection to computer
- USB-C power from some power source
- Gesture recognition via camera
- Some cute display to revert to
- Weather
- Linking to tasks

## Connection Scheme

- PC automatically connects to MCU via BLE when powered on (or just USB?)
- PC-side background app for transmitting tasks, weather data, etc.
- Data transmitted by request from MCU

## Part Selection

- ESP32-C3
- [Raspberry PI Vision Coprocessor (RPI Zero 2 W?)](https://www.sparkfun.com/raspberry-pi-zero-2-w.html)
- [IMX500 Camera Module (for gesture recognition)](https://www.sparkfun.com/raspberry-pi-ai-camera.html)
- [Screen](https://www.digikey.com/en/products/detail/adafruit-industries-llc/4311/10313914) - 80mA@5V
- le Cube (idk where to find this but amazon prolly has one; need 50mm x 50mm)
- PCB
- PCB BOM

### Libraries

- Microcontroller Code:
    - [DFRobot GDL](https://github.com/DFRobot/DFRobot_GDL)

## Sequence

- STARTUP: ESP32 displays loading screen (and loads almost instantly). When RPI starts up, send startup command, transition to IDLE
- IDLE: n possible sub-states.
    - AESTHETIC: some cute display / animation.
    - TASKS: display tasks, updated periodically from PC data transmission OR raspberry pi
    - WEATHER: display the weather. Updated periodically from PC data transmission OR raspberry pi

## THINGS TO DO:

- (electrical) ESP32 bridge with Screen
- Test screen refresh rate @ 15FPS
- Write animations & state machine for MCU
- Write background application to help fetch weather & task data
- (electrical) Create PCB for ESP32, taking into account power, bridge to screen, bridge to Raspberry PI, heat & fan (?),
- find a cube lol
- (mechanical) Design case

- Weather implementation
    - Create screens for:
        - Clear
        - Cloudy
        - Rainy (done)
        - Snowy
        - Stormy
