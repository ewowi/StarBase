#include <vector>

#include "ModulePrint.h"
#include "moduleFileServer.h"
#include "ModuleWebServer.h"
#include "moduleUI.h"

std::vector<Module *> modules; 

//setup all modules
void setup() {

  print = new ModulePrint();
  file = new ModuleFileServer();
  web = new ModuleWebServer();
  ui = new ModuleUI();

  modules.push_back(print);
  modules.push_back(file);
  modules.push_back(web);
  modules.push_back(ui);
  modules.push_back(new Module("Busses"));
  modules.push_back(new Module("Effects"));
  modules.push_back(new Module("Audio"));
  modules.push_back(new Module("..."));

  for (Module *module:modules) module->setup();
}

//loop all modules
void loop() {
  for (Module *module:modules) {
    if (module->enabled && module->success) {
      module->loop();
      module->testManager();
      module->performanceManager();
      module->dataSizeManager();
      module->codeSizeManager();
    }
  }
}