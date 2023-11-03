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

#include "SysModule.h"

#include "ArduinoJson.h"

class SysModSystem:public SysModule {

public:
  char version[16] = "";

  SysModSystem();
  void setup();
  void loop();
  void loop1s();
  void loop10s();


private:
  unsigned long loopCounter = 0;

  void addResetReasonsSelect(JsonArray select);
  void addRestartReasonsSelect(JsonArray select);

};

static SysModSystem *sys;