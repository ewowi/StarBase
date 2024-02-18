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
void Leds::setPixelColor(uint16_t indexV, CRGB color, uint8_t blendAmount) {
  if (mappingTable.size()) {
    if (indexV >= mappingTable.size()) {
      // USER_PRINTF(" dev sPC V:%d >= %d", indexV, mappingTable.size());
    }
    else {
      for (uint16_t indexP:mappingTable[indexV]) {
        fixture->ledsP[indexP] = blend(color, fixture->ledsP[indexP], blendAmount==UINT8_MAX?fixture->globalBlend:blendAmount);
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
    else if (!mappingTable[indexV].size()) //if no physMap // Core  1 panic'ed (LoadProhibited). Exception was unhandled. - std::vector<unsigned short, std::allocator<unsigned short> >::size() 
                                            // by blurrows CRGB cur = getPixelColor(XY(i,row));? XY(i,row) = 0
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

  for (std::vector<std::vector<uint16_t>>::iterator physMap=mappingTable.begin(); physMap!=mappingTable.end(); ++physMap) {
    for (uint16_t indexP:*physMap) {
      CRGB oldValue = fixture->ledsP[indexP];
      fixture->ledsP[indexP].nscale8(255-fadeBy); //this overrides the old value
      fixture->ledsP[indexP] = blend(fixture->ledsP[indexP], oldValue, fixture->globalBlend); // we want to blend in the old value
    }
  }
}

void Leds::fill_solid(const struct CRGB& color) {
  //fade2black for old start to endpos
  for (std::vector<std::vector<uint16_t>>::iterator physMap=mappingTable.begin(); physMap!=mappingTable.end(); ++physMap) {
    for (uint16_t indexP:*physMap) {
      fixture->ledsP[indexP] = blend(color, fixture->ledsP[indexP], fixture->globalBlend);
    }
  }
}

void Leds::fill_rainbow(uint8_t initialhue, uint8_t deltahue) {
  CHSV hsv;
  hsv.hue = initialhue;
  hsv.val = 255;
  hsv.sat = 240;

  for (std::vector<std::vector<uint16_t>> ::iterator physMap=mappingTable.begin(); physMap!=mappingTable.end(); ++physMap) {
    for (uint16_t indexP:*physMap) {
      fixture->ledsP[indexP] = blend(hsv, fixture->ledsP[indexP], fixture->globalBlend);
    }
    hsv.hue += deltahue;
  }
}
