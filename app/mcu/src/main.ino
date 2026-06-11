
#include "./rendering/Screen.h"
#include "./rendering/screens/LoadingScreen.h"
#include "./rendering/screens/SlidingScreen.h"
#include "./rendering/screens/SpriteScreen.h"
#include "./rendering/screens/chicken/ChickenScreen.h"
#include "./rendering/screens/work/weather/RainScreen.h"
#include "./rendering/screens/work/weather/WeatherScreen.h"
#include <Adafruit_ST7789.h>
#include <Adafruit_ImageReader.h>
#include "./data/PacketComm.h"
#include "./DeviceState.h"
#include "./asset/AssetPool.h"

// Serial pins for PI comms
#define SER_RX1 19
#define SER_TX1 18

// Custom SPI pins
#define CUSTOM_SCK  4
#define CUSTOM_MISO 5
#define CUSTOM_MOSI 6
#define CUSTOM_CS   -1

// Create a SPI object (using VSPI/SPI3, or FSPI/SPI2)
SPIClass spi = SPIClass(FSPI); // You can also use HSPI

#define TFT_DC    10
#define TFT_CS    7
#define TFT_RST   -1
#define TFT_SD_CS 1

Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST);
SdFat                SD;         // SD card filesystem
Adafruit_ImageReader reader(SD); 

Screen screen(&tft, 15); 


// packet receiver
PacketComm packetReceiverPC(Serial); 
PacketComm packetReceiverPI(Serial1); 

// make device
CubeDevice device(&screen, &reader); 


void setup() {

  Serial.begin(115200);

  // Initialize Serial1 to communicate with the Raspberry Pi
  // Parameters: baud rate, protocol configuration, RX pin, TX pin
  Serial1.begin(115200, SERIAL_8N1, SER_RX1, SER_TX1);

  // Begin SPI with custom pinout: (SCK, MISO, MOSI, SS)
  spi.begin(CUSTOM_SCK, CUSTOM_MISO, CUSTOM_MOSI, CUSTOM_CS);

  SD.begin(TFT_SD_CS); 
  AssetPool::init(reader); 

  tft.init(240, 320, SPI_MODE0);           // Init ST7789 320x240
  tft.setSPISpeed(40000000); 


  packetReceiverPC.registerCubeDevice(&device);
  packetReceiverPI.registerCubeDevice(&device);
  
}


void loop() {

  packetReceiverPC.loop(); 
  packetReceiverPI.loop();

  device.loop(); 

  
}