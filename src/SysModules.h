/*
   @title     StarMod
   @file      SysModules.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include "SysModule.h"

#include <vector>

class SysModules {
public:
  static bool newConnection;// = false; //need to be static otherwise crash
  static bool isConnected;// = false; //need to be static otherwise crash

  SysModules();

  void setup();

  void loop();

  void reboot();

  void add(SysModule* module);

  void connectedChanged();

private:
  std::vector<SysModule *> modules;
  // unsigned long oneSecondMillis = 0;
  // unsigned long tenSecondMillis = millis() - 4500;
};

static SysModules *mdls;