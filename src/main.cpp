#include <vector>

#include "module.h"

std::vector<Module> modules; 

void setup() {
  Serial.begin(115200);
  modules.push_back(Module("Serial"));
  modules.push_back(Module("Webserver"));
  modules.push_back(Module("UI"));
  modules.push_back(Module("Wifi"));
  modules.push_back(Module("Busses"));
  modules.push_back(Module("Effects"));
  modules.push_back(Module("Audio"));
  modules.push_back(Module("..."));

  for (Module module:modules) module.setup();
}

void loop() {
  for (Module module:modules) {
    module.loop();
    module.testManager();
    module.performanceManager();
    module.dataSizeManager();
    module.codeSizeManager();
  }
  Serial.println();
  delay(1000);
}