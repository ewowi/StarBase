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

enum Projections
{
  p_None,
  p_Random,
  p_DistanceFromPoint,
  p_DistanceFromCentre,
  p_Reverse,
  p_Mirror,
  p_Multiply,
  p_Kaleidoscope,
  p_Fun,
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

  uint16_t nrOfLedsP = 64; //amount of physical leds
  uint16_t nrOfLedsV = 64;  //amount of virtual leds (calculated by projection)

  uint16_t widthP = 8; 
  uint16_t heightP = 8; 
  uint16_t depthP = 1; 
  uint16_t widthV = 8; 
  uint16_t heightV = 8; 
  uint16_t depthV = 1; 

  uint8_t projectionNr = -1;
  uint8_t fixtureNr = -1;
  uint8_t effectDimension = -1;

  //track pins and leds
  uint8_t currPin;
  uint16_t prevLeds;
  uint16_t pointX, pointY, pointZ; //for distance from 

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
  std::vector<std::vector<uint16_t>> mappingTable;
  uint16_t mappingTableLedCounter;
};

//Global vars!
//after header split they all needs to be static otherwise multiple definition link error
static LedsV ledsV = LedsV(); //virtual leds
static CRGB *ledsP = ledsV.ledsPhysical; //physical leds, used by FastLed in particular
