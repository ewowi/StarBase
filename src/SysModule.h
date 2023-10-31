/*
   @title     StarMod
   @file      SysModule.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include <Arduino.h>

#include "ArduinoJson.h"

class SysModule {

public:
  const char * name;
  bool success;
  bool isEnabled;

  JsonObject parentVar;

  SysModule(const char * name) {
    this->name = name;
    success = true;
    isEnabled = true;
  }

  virtual void setup() {}

  virtual void loop() {}
  virtual void loop1s() {}
  virtual void loop10s() {}

  virtual void connectedChanged() {onOffChanged();}
  virtual void enabledChanged() {onOffChanged();}
  virtual void onOffChanged() {}

  virtual void testManager() {}
  virtual void performanceManager() {}
  virtual void dataSizeManager() {}
  virtual void codeSizeManager() {}
};