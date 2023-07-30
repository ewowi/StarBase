/*
   @title     StarMod
   @file      Modules.cpp
   @date      20230729
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "Modules.h"
#include "Sys/SysModPrint.h"

bool Modules::newConnection = false;

void Modules::setup() {
  for (Module *module:modules) {
    module->setup();
  }
}

void Modules::loop() {
  for (Module *module:modules) {
    if (module->enabled && module->success) {
      module->loop();
      // module->testManager();
      // module->performanceManager();
      // module->dataSizeManager();
      // module->codeSizeManager();
    }
  }
  if (newConnection) {
    connected();
    newConnection = false;
  }
}

void Modules::add(Module* module) {
  modules.push_back(module);
}

void Modules::connected() {
  for (Module *module:modules) {
    module->connected();
  }
}