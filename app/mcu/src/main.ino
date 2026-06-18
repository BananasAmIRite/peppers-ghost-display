
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
#include "./data/UARTComms.h"
#include "./CubeDevice.h"
#include "./asset/AssetPool.h"
#include <Arduino.h>
#include "data/SPIStream.h"


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

#define RPI_SCK 12
#define RPI_MOSI 11
#define RPI_MISO 13
#define RPI_CS 10

// Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST);
Adafruit_HX8357 tft = Adafruit_HX8357(
  &spi, TFT_CS, TFT_DC, TFT_RST
);
SdFat                SD;         // SD card filesystem
Adafruit_ImageReader reader(SD); 

Screen screen(&tft, 15); 


// packet receiver
UARTComms packetReceiverPC(Serial); 
// UARTComms packetReceiverPI(Serial1); 
// SPITransport spiReceiverPI
      // SPI3_HOST,    // SPI Host Peripherals Select
      //   RPI_MOSI, RPI_MISO, RPI_CS, RPI_SCK, 
      //   4096, 
      //   4096, 
      //   4*1024*1024
SPIStream spiReceiverPI(SPI3_HOST, 15, 16); 


// make device
CubeDevice device(&screen, &reader, &packetReceiverPC, &spiReceiverPI);

SdSpiConfig config(
    TFT_SD_CS,
    SHARED_SPI,
    SD_SCK_MHZ(20),
    &spi
);

void setup() {

  Serial.begin(115200);

  // Initialize Serial1 to communicate with the Raspberry Pi
  // Parameters: baud rate, protocol configuration, RX pin, TX pin
  // Serial1.begin(115200, SERIAL_8N1, SER_RX1, SER_TX1);

  // Begin SPI with custom pinout: (SCK, MISO, MOSI, SS)

  spi.begin(CUSTOM_SCK, CUSTOM_MISO, CUSTOM_MOSI, CUSTOM_CS);


  SD.begin(config);
  AssetPool::init(reader); 


  tft.begin(); 
  // tft.init(240, 320, SPI_MODE0);           // Init ST7789 320x240
  tft.setSPISpeed(40000000); 

  // rpispi.begin(); 

  Serial.println("starting...");
  spiReceiverPI.begin(RPI_SCK, RPI_MISO, RPI_MOSI, RPI_CS); 



  packetReceiverPC.addUARTHandler(&device);
  // packetReceiverPI.registerCubeDevice(&device);

  spiReceiverPI.addHandler(&device); 
  
  // AssetPool::instance().acquire("/chicken0.bmp");
}


void loop() {

    // if (rpispi.receiveImage())
    // {
    //     auto* canvas = rpispi.canvas();

    //     // tft.drawRGBBitmap(
    //     //     0,
    //     //     0,
    //     //     canvas->getBuffer(),
    //     //     canvas->width(),
    //     //     canvas->height());
    //     Serial.println("received image!");

    //     // Serial.println(canvas->width()); 
    //     // Serial.println(canvas->height()); 
    // }

    // Serial.println("Heartbeat"); 

    // delay(1000);
  // if (device_suspended) {
  //   device.setState(WORK);
  // } else {
  //   device.setState(IDLE); 
  // }
  
  packetReceiverPC.loop(); 
  // packetReceiverPI.loop();

  spiReceiverPI.loop(); 

  device.loop(); 
  delay(1);
}