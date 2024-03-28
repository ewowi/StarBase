/*
   @title     StarMod
   @file      SysModPins.h
   @date      20240226
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"

//info stored per pin
struct PinObject {
  char owner[32]; //if not "" then allocated (tbd: no use char)
  char details[32]; //info about pin usage
};

class SysModPins:public SysModule {

public:

  PinObject pinObjects[NUM_DIGITAL_PINS]; //all pins

  SysModPins();
  void setup();
  void loop1s();

  void allocatePin(unsigned8 pinNr, const char * owner, const char * details);
  void deallocatePin(unsigned8 pinNr, const char * owner);
  bool isOwner(unsigned8 pinNr, const char * owner) {
    return strcmp(pinObjects[pinNr].owner, owner) == 0;
  }

  //temporary functions until we refactored the PinObject
  PinObject getNthAllocatedPinObject(unsigned8 rowNr) {
    stackUnsigned8 n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        if (n == rowNr)
          return pinObject;
        n++;
      }
    }
    return PinObject();
  }
  unsigned8 getNrOfAllocatedPins() {
    stackUnsigned8 n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        n++;
      }
    }
    return n;
  }
  unsigned8 getPinNr(unsigned8 rowNr) {
    stackUnsigned8 pinNr = 0;
    stackUnsigned8 n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        if (n == rowNr)
          return pinNr;
        n++;
      }
      pinNr++;
    }
    return UINT8_MAX;
  }

  static bool updateGPIO(JsonObject var, unsigned8 rowNr, unsigned8 funType);

  bool pinsChanged = false; //update pins table if pins changed
};

extern SysModPins *pins;