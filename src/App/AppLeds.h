/*
   @title     StarMod
   @file      AppLeds.h
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

#define NUM_LEDS_Max 4096

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

class Leds {

public:

  //tbd: move ledsPhysical and nrOfLedsP out of Leds
  // CRGB *leds = nullptr;
  CRGB ledsPhysical[NUM_LEDS_Max];
    // if (!leds)
  //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
  // else
  //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
  // if (leds) free(leds);
  // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
  // leds = (CRGB*)reallocarray

  uint16_t nrOfLedsP = 64; //amount of physical leds
  uint8_t fixtureNr = -1;
  Coord3D sizeP = {8,8,1};

  
  uint16_t nrOfLeds = 64;  //amount of virtual leds (calculated by projection)

  Coord3D size = {8,8,1};
  
  uint8_t fx = -1;
  uint8_t projectionNr = -1;
  uint8_t effectDimension = -1;

  //track pins and leds
  uint8_t currPin;
  uint16_t prevLeds;
  Coord3D startPos = {0,0,0}, endPos = {8,8,1}; //default

  float distance(uint16_t x1, uint16_t y1, uint16_t z1, uint16_t x2, uint16_t y2, uint16_t z2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
  }

  uint16_t XY( uint8_t x, uint8_t y) {
    return x + y * size.x;
  }
  uint16_t XYZ( uint8_t x, uint8_t y, uint8_t z) {
    return x + y * size.x + z * size.x * size.y;
  }

  void fixtureProjectAndMap();

  uint16_t indexVLocal = 0; //set in operator[], used by operator=

  // indexVLocal stored to be used by other operators
  Leds& operator[](uint16_t indexV) {
    indexVLocal = indexV;
    return *this;
  }

  Leds& operator[](Coord3D pos) {
    indexVLocal = XYZ(pos.x, pos.y, pos.z);
    return *this;
  }

  // CRGB& operator[](uint16_t indexV) {
  //   // indexVLocal = indexV;
  //   CRGB x = getPixelColor(indexV);
  //   return x;
  // }

  // uses indexVLocal and color to call setPixelColor
  Leds& operator=(const CRGB color) {
    setPixelColor(indexVLocal, color);
    return *this;
  }

  Leds& operator+=(const CRGB color) {
    setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
    return *this;
  }
  Leds& operator|=(const CRGB color) {
    // setPixelColor(indexVLocal, color);
    setPixelColor(indexVLocal, getPixelColor(indexVLocal) | color);
    return *this;
  }

  // Leds& operator+(const CRGB color) {
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
  //   return *this;
  // }


  // maps the virtual led to the physical led(s) and assign a color to it
  void setPixelColor(int indexV, CRGB color) {
    if (mappingTable.size()) {
      if (indexV >= mappingTable.size()) return;
      for (uint16_t indexP:mappingTable[indexV]) {
        if (indexP < NUM_LEDS_Max)
          ledsPhysical[indexP] = color;
      }
    }
    else //no projection
      ledsPhysical[projectionNr==p_Random?random(nrOfLedsP):indexV] = color;
  }

  CRGB getPixelColor(int indexV) {
    if (mappingTable.size()) {
      if (indexV >= mappingTable.size()) return CRGB::Black;
      if (!mappingTable[indexV].size() || mappingTable[indexV][0] > NUM_LEDS_Max) return CRGB::Black;

      return ledsPhysical[mappingTable[indexV][0]]; //any would do as they are all the same
    }
    else //no projection
      return ledsPhysical[indexV];
  }

  void addPixelColor(int indexV, CRGB color) {
    setPixelColor(indexV, getPixelColor(indexV) + color);
  }

  void fadeToBlackBy(uint8_t fadeBy = 255) {
    //fade2black for old start to endpos
    Coord3D index;
    for (index.x = startPos.x; index.x <= endPos.x; index.x++)
      for (index.y = startPos.y; index.y <= endPos.y; index.y++)
        for (index.z = startPos.z; index.z <= endPos.z; index.z++) {
          ledsPhysical[index.x + index.y * sizeP.x + index.z * sizeP.x * sizeP.y].nscale8(255-fadeBy);
        }
  }
  void fill_solid(const struct CRGB& color) {
    //fade2black for old start to endpos
    Coord3D index;
    for (index.x = startPos.x; index.x <= endPos.x; index.x++)
      for (index.y = startPos.y; index.y <= endPos.y; index.y++)
        for (index.z = startPos.z; index.z <= endPos.z; index.z++) {
          ledsPhysical[index.x + index.y * sizeP.x + index.z * sizeP.x * sizeP.y] = color;
        }
  }

  void fill_rainbow(uint8_t initialhue, uint8_t deltahue) {
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;
    Coord3D index;
    for (index.x = startPos.x; index.x <= endPos.x; index.x++)
      for (index.y = startPos.y; index.y <= endPos.y; index.y++)
        for (index.z = startPos.z; index.z <= endPos.z; index.z++) {
          ledsPhysical[index.x + index.y * sizeP.x + index.z * sizeP.x * sizeP.y] = hsv;
          hsv.hue += deltahue;
        }
  }

  void blur2d(fract8 blur_amount)
  {
      blurRows(size.x, size.y, blur_amount);
      blurColumns(size.x, size.y, blur_amount);
  }

  void blurRows(uint8_t width, uint8_t height, fract8 blur_amount)
  {
  /*    for( uint8_t row = 0; row < height; row++) {
          CRGB* rowbase = leds + (row * width);
          blur1d( rowbase, width, blur_amount);
      }
  */
      // blur rows same as columns, for irregular matrix
      uint8_t keep = 255 - blur_amount;
      uint8_t seep = blur_amount >> 1;
      for( uint8_t row = 0; row < height; row++) {
          CRGB carryover = CRGB::Black;
          for( uint8_t i = 0; i < width; i++) {
              CRGB cur = getPixelColor(XY(i,row));
              CRGB part = cur;
              part.nscale8( seep);
              cur.nscale8( keep);
              cur += carryover;
              if( i) addPixelColor(XY(i-1,row), part);
              setPixelColor(XY(i,row), cur);
              carryover = part;
          }
      }
  }

  // blurColumns: perform a blur1d on each column of a rectangular matrix
  void blurColumns(uint8_t width, uint8_t height, fract8 blur_amount)
  {
      // blur columns
      uint8_t keep = 255 - blur_amount;
      uint8_t seep = blur_amount >> 1;
      for( uint8_t col = 0; col < width; ++col) {
          CRGB carryover = CRGB::Black;
          for( uint8_t i = 0; i < height; ++i) {
              CRGB cur = getPixelColor(XY(col,i));
              CRGB part = cur;
              part.nscale8( seep);
              cur.nscale8( keep);
              cur += carryover;
              if( i) addPixelColor(XY(col,i-1), part);
              setPixelColor(XY(col,i), cur);
              carryover = part;
          }
      }
  }

private:
  std::vector<std::vector<uint16_t>> mappingTable;
  uint16_t mappingTableLedCounter;
};