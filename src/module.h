#include <Arduino.h>

class Module {

public:
  const char *name;

  Module(const char *name) {
    this->name = name;
  }

  void setup() {

  }

  void loop() {
        Serial.printf("%s ", name);
  }

  void testManager() {}
  void performanceManager() {}
  void dataSizeManager() {}
  void codeSizeManager() {}
};