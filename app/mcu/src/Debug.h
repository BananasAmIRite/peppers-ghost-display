#pragma once
#include <HardwareSerial.h>

// #define DEBUG

#ifdef DEBUG
  #define LOG(x)   Serial.print(x)
  #define LOGLN(x) Serial.println(x)
#else
  #define LOG(x)
  #define LOGLN(x)
#endif