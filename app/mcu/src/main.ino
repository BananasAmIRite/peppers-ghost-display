
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

// screen definitions

LoadingScreen loadingScreen(333); 
// LoadingScreen loadingScreen3(333); 
// SpriteScreen idleScreen(reader, idleAnimation, 100, true); 
// ChickenScreen idleScreen(reader); 
// RainScreen idleScreen(reader);
ChickenScreen idleScreen(reader); 
std::shared_ptr<WeatherScreen> weatherScreen(new WeatherScreen(reader));
std::shared_ptr<Renderable> tasksScreen(new EmptyScreen());

SlidingScreen workScreen({
  weatherScreen, 
  tasksScreen
}); 

// packet receiver
PacketComm packetReceiver; 

// make device
CubeDevice device(&screen, &reader, loadingScreen, idleScreen, workScreen); 

void setup() {

  Serial.begin(115200);

  // Begin SPI with custom pinout: (SCK, MISO, MOSI, SS)
  spi.begin(CUSTOM_SCK, CUSTOM_MISO, CUSTOM_MOSI, CUSTOM_CS);

  SD.begin(TFT_SD_CS); 
  
  tft.init(240, 320, SPI_MODE0);           // Init ST7789 320x240
  tft.setSPISpeed(40000000); 

  packetReceiver.registerCubeDevice(&device); 
  

  // Serial.println(reader.loadBMP("/chickens.bmp", image)); 

  weatherScreen->updateWeather(82, 65, 67, Storm); 

  
}


void loop() {

  while (Serial.available())
  {
      packetReceiver.processByte(Serial.read());
  }
  screen.tryRender(); 
  device.loop(); 
  // Serial.println("running..."); 

  // drawLoadedBMPToGFX(image, *screen.getScreen(), 0, 0, 8); 

  // screen.update(); 

  // delay(1000);
}