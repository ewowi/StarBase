#pragma once

#include <vector>

class Modules {
public:
  std::vector<Module *> modules;

  void setup() {
      for (Module *module:modules) module->setup();
  }

  void loop() {
    for (Module *module:modules) {
      if (module->enabled && module->success) {
        module->loop();
        // module->testManager();
        // module->performanceManager();
        // module->dataSizeManager();
        // module->codeSizeManager();
      }
    }
  }

  void add (Module module) {
    // modules.push_back(module);
  }

  void connected() {
    for (Module *module:modules) module->connected();
  }

};

static Modules *mdls;