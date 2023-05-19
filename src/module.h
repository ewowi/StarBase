#include <Arduino.h>

class Module {

public:
  const char *name;

  Module(const char *name) {
    this->name = name;
  }

  virtual void setup() {
    Serial.printf("setup %s ", name);
  }

  virtual void loop() {
    Serial.printf("%s ", name);
  }

  virtual void testManager() {}
  virtual void performanceManager() {}
  virtual void dataSizeManager() {}
  virtual void codeSizeManager() {}
};