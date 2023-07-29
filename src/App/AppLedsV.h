/*
   @title     StarMod
   @file      AppLedsV.h
   @date      20230729
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "FastLED.h"
#include <vector>
#include "ArduinoJson.h"

#define NUM_LEDS_FastLed 1024
#define NUM_LEDS_Preview 2000

//keep them global for the time being as FastLed effects refer to them and want to keep that code as unchanged as possible
//(so maybe move there?)

static float distance(uint16_t x1, uint16_t y1, uint16_t z1, uint16_t x2, uint16_t y2, uint16_t z2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
}

class LedsV {

public:
  // CRGB *leds = nullptr;
  CRGB ledsP[NUM_LEDS_Preview];

  static uint16_t nrOfLedsP; //amount of physical leds
  static uint16_t nrOfLedsV;  //amount of virtual leds (calculated by projection)

  static uint16_t width; 
  static uint16_t height; 
  static uint16_t depth; 

  //need to make these static as they are called in lambda functions
  static std::vector<std::vector<uint16_t>> mappingTable;
  static uint16_t mappingTableLedCounter;

  void ledFixProjectAndMap(JsonObject ledFixObject, JsonObject projectionObject);

  uint16_t indexVLocal = 0;

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

  // LedsV& operator+=(const CRGB color) {
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
  //   return *this;
  // }
  // LedsV& operator|=(const CRGB color) {
  //   // setPixelColor(indexVLocal, color);
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) | color);
  //   return *this;
  // }

  // LedsV& operator+(const CRGB color) {
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
  //   return *this;
  // }

};

//after header split they all needs to be static otherwise multiple definition link error
static LedsV ledsV = LedsV(); //virtual leds
static CRGB *ledsP = ledsV.ledsP; //physical leds, used by FastLed in particular