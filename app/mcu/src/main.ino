
#include "./rendering/Screen.h"
#include "./rendering/screens/LoadingScreen.h"
#include "./rendering/screens/SlidingScreen.h"
#include "./rendering/screens/SpriteScreen.h"
#include "./rendering/screens/chicken/ChickenScreen.h"
#include "./rendering/screens/work/weather/RainScreen.h"
#include "./rendering/screens/work/weather/WeatherScreen.h"
// #include <Adafruit_ST7789.h>
#include <Adafruit_HX8357.h>
#include <Adafruit_ImageReader.h>
#include "./data/PacketComm.h"
#include "./DeviceState.h"
#include "./asset/AssetPool.h"
#include <Arduino.h>

// Serial pins for PI comms
// #define SER_RX1 19
// #define SER_TX1 18

// Custom SPI pins
#define CUSTOM_SCK  39
#define CUSTOM_MISO 40
#define CUSTOM_MOSI 41
#define CUSTOM_CS   -1

// Create a SPI object (using VSPI/SPI3, or FSPI/SPI2)
SPIClass spi = SPIClass(FSPI); // You can also use HSPI

#define TFT_DC    2
#define TFT_CS    42
#define TFT_RST   -1
#define TFT_SD_CS 1
// int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK,
//                   int8_t _RST, int8_t _MISO
// Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST);
Adafruit_HX8357 tft = Adafruit_HX8357(
  &spi, TFT_CS, TFT_DC, TFT_RST
  // TFT_CS, TFT_DC, CUSTOM_MOSI, CUSTOM_SCK, TFT_RST, CUSTOM_MISO
);
SdFat                SD;         // SD card filesystem
Adafruit_ImageReader reader(SD); 

Screen screen(&tft, 15); 


// packet receiver
PacketComm packetReceiverPC(Serial); 
// PacketComm packetReceiverPI(Serial1); 

// // make device
CubeDevice device(&screen, &reader);

// bool device_suspended = false; 

SdSpiConfig config(
    TFT_SD_CS,
    SHARED_SPI,
    SD_SCK_MHZ(20),
    &spi
);

void setup() {

  Serial.begin(115200);

  // // Initialize Serial1 to communicate with the Raspberry Pi
  // // Parameters: baud rate, protocol configuration, RX pin, TX pin
  // Serial1.begin(115200, SERIAL_8N1, SER_RX1, SER_TX1);

  // Begin SPI with custom pinout: (SCK, MISO, MOSI, SS)

  spi.begin(CUSTOM_SCK, CUSTOM_MISO, CUSTOM_MOSI, CUSTOM_CS);


  SD.begin(config);
  AssetPool::init(reader); 


  tft.begin(); 
  // tft.init(240, 320, SPI_MODE0);           // Init ST7789 320x240
  tft.setSPISpeed(40000000); 


  packetReceiverPC.registerCubeDevice(&device);
  // packetReceiverPI.registerCubeDevice(&device);
  
  // AssetPool::instance().acquire("/chicken0.bmp");
}


void loop() {

  // if (device_suspended) {
  //   device.setState(WORK);
  // } else {
  //   device.setState(IDLE); 
  // }
  
  packetReceiverPC.loop(); 
  // packetReceiverPI.loop();

  device.loop(); 
  
}

// #include <Arduino.h>

// extern "C" void tud_suspend_cb(bool remote_wakeup_en)
// {
//   device_suspended = true; 
// }

// extern "C" void tud_resume_cb(void)
// {
//   device_suspended = false; 
// }
