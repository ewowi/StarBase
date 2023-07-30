/*
   @title     StarMod
   @file      SysModPins.h
   @date      20230729
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "Module.h"

#include "ArduinoJson.h"

class SysModPins:public Module {

public:

  SysModPins();
  void setup();
  void loop();

  void registerPin(uint8_t pinNr);

  static void updateGPIO(JsonObject var);

};

static SysModPins *pin;