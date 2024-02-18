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

#pragma once


#include "../Sys/SysModModel.h" //for Coord3D

#include "AppLeds.h"

#define NUM_LEDS_Max 4096

#define _1D 1
#define _2D 2
#define _3D 3

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
  p_count
};


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

  std::vector<Leds> ledsList; //virtual leds

  Coord3D head = {0,0,0};

  //variables for json Scan
  // uint16_t prevIndexP;
  //track pins and leds

  bool doMap = false;

  uint8_t globalBlend = 128;
  
  //load fixture json file, parse it and depending on the projection, create a mapping for it
  void projectAndMap();

  float distance(uint16_t x1, uint16_t y1, uint16_t z1, uint16_t x2, uint16_t y2, uint16_t z2) {
    return distance(Coord3D{x1, y1, z1}, Coord3D{x2,y2,z2});
    // return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
  }
  float distance(Coord3D c1, Coord3D c2) {
    Coord3D delta = (c1-c2).absx();
    return sqrtf((delta.x)*(delta.x) + (delta.y)*(delta.y) + (delta.z)*(delta.z));
  }

};
