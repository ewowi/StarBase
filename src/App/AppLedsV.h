/*
   @title     StarMod
   @file      AppLedsV.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "FastLED.h"
#include <vector>
#include "ArduinoJson.h"

#define NUM_LEDS_Preview 8192

//keep them global for the time being as FastLed effects refer to them and want to keep that code as unchanged as possible
//(so maybe move there?)

static float distance(uint16_t x1, uint16_t y1, uint16_t z1, uint16_t x2, uint16_t y2, uint16_t z2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
}

enum Projections
{
  p_None,
  p_Random,
  p_DistanceFromPoint,
  p_DistanceFromCentre,
  count
};

struct Coordinate {
  uint16_t x;
  uint16_t y;
  uint16_t z;
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

  static uint16_t nrOfLedsP; //amount of physical leds
  static uint16_t nrOfLedsV;  //amount of virtual leds (calculated by projection)

  static uint16_t widthP; 
  static uint16_t heightP; 
  static uint16_t depthP; 
  static uint16_t widthV; 
  static uint16_t heightV; 
  static uint16_t depthV; 

  static uint8_t projectionNr;
  static uint8_t ledFixNr;
  static uint8_t fxDimension;

  void ledFixProjectAndMap();

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

private:
  //need to make these static as they are called in lambda functions
  static std::vector<std::vector<uint16_t>> mappingTable;
  static uint16_t mappingTableLedCounter;
};

//Global vars!
//after header split they all needs to be static otherwise multiple definition link error
static LedsV ledsV = LedsV(); //virtual leds
static CRGB *ledsP = ledsV.ledsPhysical; //physical leds, used by FastLed in particular
