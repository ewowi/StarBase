/*
   @title     StarMod
   @file      SysModPins.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModule.h"

#include "ArduinoJson.h"

#define NUM_PINS 50

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

  static void updateGPIO(JsonObject var, size_t index);

private:
  static bool pinsChanged; //update pins table if pins changed
};

static SysModPins *pins;