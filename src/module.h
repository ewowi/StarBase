#include <Arduino.h>

#include "ArduinoJson.h"

#pragma once //included by multiple files

class Module {

public:
  const char *name;
  bool success;
  bool enabled;

  JsonObject parentObject;

  Module(const char *name) {
    this->name = name;
    enabled = true;
  }

  virtual void setup() {
    success = true;
  }

  virtual void loop() {
  }

  virtual void connected() {}

  virtual void testManager() {}
  virtual void performanceManager() {}
  virtual void dataSizeManager() {}
  virtual void codeSizeManager() {}
};