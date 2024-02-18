/*
   @title     StarMod
   @file      SysModPins.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"

#define NUM_PINS NUM_DIGITAL_PINS // softhack007 NUM_DIGITAL_PINS comes from the arduino framework

//info stored per pin
struct PinObject {
  char owner[32]; //if not "" then allocated (tbd: no use char)
  char details[32]; //info about pin usage
};

class SysModPins:public SysModule {

public:

  static PinObject pinObjects[NUM_PINS]; //all pins

  SysModPins();
  void setup();
  void loop1s();

  void allocatePin(uint8_t pinNr, const char * owner, const char * details);
  void deallocatePin(uint8_t pinNr, const char * owner);

  //temporary functions until we refactored the PinObject
  PinObject getNthAllocatedPinObject(uint8_t rowNr) {
    uint8_t n = 0;
    for (PinObject pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        if (n == rowNr)
          return pinObject;
        n++;
      }
    }
    return PinObject();
  }
  uint8_t getNrOfAllocatedPins() {
    uint8_t n = 0;
    for (PinObject pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        n++;
      }
    }
    return n;
  }
  uint8_t getPinNr(uint8_t rowNr) {
    uint8_t pinNr = 0;
    uint8_t n = 0;
    for (PinObject pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        if (n == rowNr)
          return pinNr;
        n++;
      }
      pinNr++;
    }
    return UINT8_MAX;
  }

  static bool updateGPIO(JsonObject var, uint8_t rowNr, uint8_t funType);

private:
  static bool pinsChanged; //update pins table if pins changed
};

static SysModPins *pins;