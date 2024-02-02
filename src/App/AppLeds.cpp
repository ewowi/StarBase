/*
   @title     StarMod
   @file      AppModLeds.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "AppLeds.h"

// maps the virtual led to the physical led(s) and assign a color to it
void Leds::setPixelColor(uint16_t indexV, CRGB color) {
  if (mappingTable.size()) {
    if (indexV >= mappingTable.size()) {
      USER_PRINTF(" dev sPC V:%d >= %d", indexV, mappingTable.size());
    }
    else {
      for (uint16_t indexP:mappingTable[indexV]) {
        fixture->ledsP[indexP] = blend(color, fixture->ledsP[indexP], 128); //more new color (75%) then old color (25%)
      }
    }
  }
  else if (indexV<NUM_LEDS_Max)//no projection
    fixture->ledsP[projectionNr==p_Random?random(fixture->nrOfLeds):indexV] = color;
  else
    USER_PRINTF(" dev sPC V:%d >= %d", indexV, NUM_LEDS_Max);
}

CRGB Leds::getPixelColor(uint16_t indexV) {
  if (mappingTable.size()) {
    if (indexV >= mappingTable.size()) {
      USER_PRINTF(" dev gPC V %d >= %d", indexV, mappingTable.size());
      return CRGB::Black;
    }
    else if (!mappingTable[indexV].size()) //if no physMap
    {
      USER_PRINTF(" dev gPC P:%d >= %d", mappingTable[indexV][0], NUM_LEDS_Max);
      return CRGB::Black;
    }
    else
      return fixture->ledsP[mappingTable[indexV][0]]; //any would do as they are all the same
  }
  else if (indexV < NUM_LEDS_Max) //no mapping
    return fixture->ledsP[indexV];
  else {
    USER_PRINTF(" dev gPC N: %d >= %d", indexV, NUM_LEDS_Max);
    return CRGB::Black;
  }
}

void Leds::fadeToBlackBy(uint8_t fadeBy) {
  //fade2black for old start to endpos
  Coord3D index;
  Coord3D max = {min(endPos.x, (uint16_t)(fixture->size.x-1)), min(endPos.y, (uint16_t)(fixture->size.y-1)), min(endPos.z, (uint16_t)(fixture->size.z-1))};
  for (index.x = startPos.x; index.x <= max.x; index.x++)
    for (index.y = startPos.y; index.y <= max.y; index.y++)
      for (index.z = startPos.z; index.z <= max.z; index.z++) {
        uint16_t indexP = index.x + index.y * fixture->size.x + index.z * fixture->size.x * fixture->size.y;
        if (indexP < NUM_LEDS_Max) fixture->ledsP[indexP].nscale8(255-fadeBy);
        else
          USER_PRINTF(" dev fB %d >= %d", indexP, NUM_LEDS_Max);
      }
}

void Leds::fill_solid(const struct CRGB& color) {
  //fade2black for old start to endpos
  Coord3D index;
  Coord3D max = {min(endPos.x, (uint16_t)(fixture->size.x-1)), min(endPos.y, (uint16_t)(fixture->size.y-1)), min(endPos.z, (uint16_t)(fixture->size.z-1))};
  for (index.x = startPos.x; index.x <= max.x; index.x++)
    for (index.y = startPos.y; index.y <= max.y; index.y++)
      for (index.z = startPos.z; index.z <= max.z; index.z++) {
        uint16_t indexP = index.x + index.y * fixture->size.x + index.z * fixture->size.x * fixture->size.y;
        if (indexP < NUM_LEDS_Max) fixture->ledsP[indexP] = color;
        else
          USER_PRINTF(" dev fS %d >= %d", indexP, NUM_LEDS_Max);
      }
}

void Leds::fill_rainbow(uint8_t initialhue, uint8_t deltahue) {
  CHSV hsv;
  hsv.hue = initialhue;
  hsv.val = 255;
  hsv.sat = 240;
  Coord3D index;
  Coord3D max = {min(endPos.x, (uint16_t)(fixture->size.x-1)), min(endPos.y, (uint16_t)(fixture->size.y-1)), min(endPos.z, (uint16_t)(fixture->size.z-1))};
  for (index.x = startPos.x; index.x <= max.x; index.x++)
    for (index.y = startPos.y; index.y <= max.y; index.y++)
      for (index.z = startPos.z; index.z <= max.z; index.z++) {
        uint16_t indexP = index.x + index.y * fixture->size.x + index.z * fixture->size.x * fixture->size.y;
        if (indexP < NUM_LEDS_Max) fixture->ledsP[indexP] = hsv;
        else
          USER_PRINTF(" dev fR %d >= %d", indexP, NUM_LEDS_Max);
        hsv.hue += deltahue;
      }
}
