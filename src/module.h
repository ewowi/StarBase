#pragma once
#include <Arduino.h>

#include "ArduinoJson.h"


class Module {

public:
  const char * name;
  bool success;
  bool enabled;
  unsigned long secondMillis = 0;

  JsonObject parentObject;

  Module(const char * name) {
    this->name = name;
    success = true;
    enabled = true;
    // Serial.printf("Constructor %s %s\n", __PRETTY_FUNCTION__, name);
  }

  virtual void setup() {
  }

  virtual void loop() {
  }

  virtual void connected() {}

  virtual void testManager() {}
  virtual void performanceManager() {}
  virtual void dataSizeManager() {}
  virtual void codeSizeManager() {}
};