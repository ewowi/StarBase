/*
   @title     StarMod
   @file      SysModModules.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include "Module.h"

#include <vector>

class Modules:public Module {
public:
  static bool newConnection; //need to be static otherwise crash

  Modules();

  void setup();

  void loop();

  void add(Module* module);

  void connected();

private:
  static std::vector<Module *> modules;
};

static Modules *mdls;