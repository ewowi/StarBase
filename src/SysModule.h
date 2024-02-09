/*
   @title     StarMod
   @file      SysModule.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

//include all system wide libraries here (so no need to replicate in multiple modules)

#include <Arduino.h>
#include <HardwareSerial.h>  // ensure we have the correct "Serial" on new MCUs (depends on ARDUINO_USB_MODE and ARDUINO_USB_CDC_ON_BOOT)
#include <WiFi.h>
#include "esp_wifi.h"
#include <LittleFS.h>        // make sure that ArduinoJson knows we have LittleFS

// #define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#include "ArduinoJson.h"

#include <vector>

class SysModule {

public:
  const char * name;
  bool success;
  bool isEnabled;
  unsigned long oneSecondMillis = millis() - random(1000); //random so not all 1s are fired at once
  unsigned long tenSecondMillis = millis() - random(10000); //random within a second

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

  virtual void reboot() {}

  virtual void connectedChanged() {onOffChanged();}
  virtual void enabledChanged() {onOffChanged();}
  virtual void onOffChanged() {}

  virtual void testManager() {}
  virtual void performanceManager() {}
  virtual void dataSizeManager() {}
  virtual void codeSizeManager() {}
};