#include <vector>
#include <WiFi.h> //needed here instead of moduleWebServer as ESPAsyncWebServer.git requires it

#include "modulePrintServer.h"
#include "moduleOSServer.h"
#include "moduleFileServer.h"
#include "moduleWebServer.h"
#include "moduleUIServer.h"
#include "moduleToggle.h"

std::vector<Module *> modules; 

//setup all modules
void setup() {

  print = new ModulePrintServer();
  os = new ModuleOSServer();
  file = new ModuleFileServer();
  web = new ModuleWebServer();
  ui = new ModuleUIServer();
  toggle = new ModuleToggle();

  modules.push_back(print);
  modules.push_back(os);
  modules.push_back(file); //90K lps (loops per second)
  modules.push_back(web); //80K lps
  modules.push_back(ui);
  modules.push_back(toggle); //70K lps
  // modules.push_back(new Module("Busses"));
  // modules.push_back(new Module("Effects"));
  // modules.push_back(new Module("Audio"));
  // modules.push_back(new Module("..."));

  for (Module *module:modules) module->setup();
}

//loop all modules
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