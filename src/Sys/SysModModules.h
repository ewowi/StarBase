/*
   @title     StarMod
   @file      SysModModules.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include "Module.h"

#include <vector>

class SysModModules:public Module {
public:
  static bool newConnection; //need to be static otherwise crash
  static bool isConnected;

  SysModModules();

  void setup();

  void loop();

  void add(Module* module);

  void connectedChanged();

private:
  static std::vector<Module *> modules;
};

static SysModModules *mdls;