/*
   @title     StarMod
   @file      AppLedsV.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

// FastLED optional flags to configure drivers, see https://github.com/FastLED/FastLED/blob/master/src/platforms/esp/32
// RMT driver (default)
// #define FASTLED_ESP32_FLASH_LOCK 1    // temporarily disabled FLASH file access while driving LEDs (may prevent random flicker)
// #define FASTLED_RMT_BUILTIN_DRIVER 1  // in case your app needs to use RMT units, too (slower)
// I2S parallel driver
// #define FASTLED_ESP32_I2S true        // to use I2S parallel driver (instead of RMT)
// #define I2S_DEVICE 1                  // I2S driver: allows to still use I2S#0 for audio (only on esp32 and esp32-s3)
// #define FASTLED_I2S_MAX_CONTROLLERS 8 // 8 LED pins should be enough (default = 24)
#include "FastLED.h"
#include <vector>
#include "ArduinoJson.h"
#include "../Sys/SysModModel.h" //for Coord3D

#define NUM_LEDS_Preview 4096

enum Projections
{
  p_None,
  p_Random,
  p_DistanceFromPoint,
  p_DistanceFromCenter,
  p_Reverse,
  p_Mirror,
  p_Multiply,
  p_Kaleidoscope,
  p_Fun,
  count
};

class LedsV {

public:
  // CRGB *leds = nullptr;
  CRGB ledsPhysical[NUM_LEDS_Preview];
    // if (!leds)
  //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
  // else
  //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
  // if (leds) free(leds);
  // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
  // leds = (CRGB*)reallocarray

  uint16_t nrOfLedsP = 64; //amount of physical leds
  uint16_t nrOfLedsV = 64;  //amount of virtual leds (calculated by projection)

  uint16_t widthP = 8; 
  uint16_t heightP = 8; 
  uint16_t depthP = 1; 
  uint16_t widthV = 8; 
  uint16_t heightV = 8; 
  uint16_t depthV = 1; 

  uint8_t fx = -1;
  uint8_t projectionNr = -1;
  uint8_t fixtureNr = -1;
  uint8_t effectDimension = -1;

  //track pins and leds
  uint8_t currPin;
  uint16_t prevLeds;
  Coord3D startPos = {0,0,0}, endPos = {8,8,1}; //default

  float distance(uint16_t x1, uint16_t y1, uint16_t z1, uint16_t x2, uint16_t y2, uint16_t z2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
  }

  uint16_t XY( uint8_t x, uint8_t y) {
    return x + y * widthV;
  }
  uint16_t XYZ( uint8_t x, uint8_t y, uint8_t z) {
    return x + y * widthV + z * widthV * heightV;
  }

  void fixtureProjectAndMap();

  uint16_t indexVLocal = 0; //set in operator[], used by operator=

  // ledsV[indexV] stores indexV locally
  LedsV& operator[](uint16_t indexV);

  // CRGB& operator[](uint16_t indexV) {
  //   // indexVLocal = indexV;
  //   CRGB x = getPixelColor(indexV);
  //   return x;
  // }

  // ledsV = uses locally stored indexV and color to call setPixelColor
  LedsV& operator=(const CRGB color);

  // maps the virtual led to the physical led(s) and assign a color to it
  void setPixelColor(int indexV, CRGB color);

  CRGB getPixelColor(int indexV);

  LedsV& operator+=(const CRGB color) {
    setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
    return *this;
  }
  LedsV& operator|=(const CRGB color) {
    // setPixelColor(indexVLocal, color);
    setPixelColor(indexVLocal, getPixelColor(indexVLocal) | color);
    return *this;
  }

  // LedsV& operator+(const CRGB color) {
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
  //   return *this;
  // }

  void fadeToBlackBy(uint8_t fadeBy = 255) {
    //fade2black for old start to endpos
    Coord3D index;
    for (index.x = startPos.x; index.x <= endPos.x; index.x++)
      for (index.y = startPos.y; index.y <= endPos.y; index.y++)
        for (index.z = startPos.z; index.z <= endPos.z; index.z++) {
          ledsPhysical[index.x + index.y * widthP + index.z * widthP * heightP].nscale8(255-fadeBy);
        }
  }
  void fill_solid(const struct CRGB& color) {
    //fade2black for old start to endpos
    Coord3D index;
    for (index.x = startPos.x; index.x <= endPos.x; index.x++)
      for (index.y = startPos.y; index.y <= endPos.y; index.y++)
        for (index.z = startPos.z; index.z <= endPos.z; index.z++) {
          ledsPhysical[index.x + index.y * widthP + index.z * widthP * heightP] = color;
        }
  }

  void fill_rainbow(uint8_t initialhue,
                  uint8_t deltahue )
{
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;
    Coord3D index;
    for (index.x = startPos.x; index.x <= endPos.x; index.x++)
      for (index.y = startPos.y; index.y <= endPos.y; index.y++)
        for (index.z = startPos.z; index.z <= endPos.z; index.z++) {
          ledsPhysical[index.x + index.y * widthP + index.z * widthP * heightP] = hsv;
          hsv.hue += deltahue;
        }
  }
private:
  std::vector<std::vector<uint16_t>> mappingTable;
  uint16_t mappingTableLedCounter;
};

//Global vars!
//after header split they all needs to be static otherwise multiple definition link error
static LedsV ledsV = LedsV(); //virtual leds
// static CRGB *ledsP = ledsV.ledsPhysical; //physical leds, used by FastLed in particular
