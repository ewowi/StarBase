#include <vector>

#include "moduleWifi.h"

std::vector<Module *> modules; 

void setup() {
  Serial.begin(115200);
  delay(4000); //needed for the time being

  modules.push_back(new Module("Serial"));
  modules.push_back(new Module("Webserver"));
  modules.push_back(new Module("UI"));
  modules.push_back(new ModuleWifi());
  modules.push_back(new Module("Busses"));
  modules.push_back(new Module("Effects"));
  modules.push_back(new Module("Audio"));
  modules.push_back(new Module("..."));

  for (Module *module:modules) module->setup();
  Serial.println();
}

void loop() {
  for (Module *module:modules) {
    module->loop();
    module->testManager();
    module->performanceManager();
    module->dataSizeManager();
    module->codeSizeManager();
  }
  Serial.println();
  delay(1000);
}