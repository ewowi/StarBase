/*
   @title     StarMod
   @file      AppFixture.h
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include "../Sys/SysModModel.h" //for Coord3D

#include "AppLeds.h"

#define NUM_LEDS_Max 4096

#define _1D 1
#define _2D 2
#define _3D 3


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

  unsigned16 nrOfLeds = 64; //amount of physical leds
  unsigned8 fixtureNr = -1;
  Coord3D size = {8,8,1};

  std::vector<Leds *> ledsList; //virtual leds

  Coord3D head = {0,0,0};

  bool doMap = false;
  bool doAllocPins = false;

  unsigned8 globalBlend = 128;
  
  //load fixture json file, parse it and depending on the projection, create a mapping for it
  void projectAndMap();

  float distance(float x1, float y1, float z1, float x2, float y2, float z2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
  }
  float distance(unsigned16 x1, unsigned16 y1, unsigned16 z1, unsigned16 x2, unsigned16 y2, unsigned16 z2) {
    return distance(Coord3D{x1, y1, z1}, Coord3D{x2,y2,z2});
    // return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
  }
  float distance(Coord3D c1, Coord3D c2) {
    Coord3D delta = (c1-c2);
    return sqrtf((delta.x)*(delta.x) + (delta.y)*(delta.y) + (delta.z)*(delta.z));
  }

};
