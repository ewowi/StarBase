/*
   @title     StarMod
   @file      AppFixture.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

// #pragma once

// #include "AppLeds.h"

#define NUM_LEDS_Max 4096

class Leds; //forward

class Fixture {

public:

  CRGB ledsP[NUM_LEDS_Max];

  // CRGB *leds = nullptr;
    // if (!leds)
  //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
  // else
  //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
  // if (leds) free(leds);
  // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
  // leds = (CRGB*)reallocarray

  uint16_t nrOfLeds = 64; //amount of physical leds
  uint8_t fixtureNr = -1;
  Coord3D size = {8,8,1};

  Leds *leds;

  Fixture() {
    // leds->distance(1,2,3,4,5,6);
  }

};
