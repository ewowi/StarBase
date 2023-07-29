/*
   @title     StarMod
   @file      SysModSystem.h
   @date      20230729
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "Module.h"

#include "ArduinoJson.h"

class SysModSystem:public Module {

public:

  SysModSystem();
  void setup();
  void loop();


private:
  unsigned long loopCounter = 0;

  void addResetReasonsSelect(JsonArray select);
  void addRestartReasonsSelect(JsonArray select);

};

static SysModSystem *sys;