/*
   @title     StarMod
   @file      SysModSystem.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once

#include "Module.h"

#include "ArduinoJson.h"

class SysModSystem:public Module {

public:
  static char version[16];

  SysModSystem();
  void setup();
  void loop();


private:
  unsigned long loopCounter = 0;
  unsigned long tenSecondMillis = 0;

  void addResetReasonsSelect(JsonArray select);
  void addRestartReasonsSelect(JsonArray select);

};

static SysModSystem *sys;