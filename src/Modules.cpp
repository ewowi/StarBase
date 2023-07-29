#include "Modules.h"
#include "sys/SysModPrint.h"

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
  for (Module *module:modules) { //this causes crash!!! why???
    module->connected();
  }
}