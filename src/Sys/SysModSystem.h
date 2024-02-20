/*
   @title     StarMod
   @file      SysModSystem.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include "SysModule.h"

class SysModSystem:public SysModule {

public:
  char version[16] = "";
  char chipInfo[64] = "";

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