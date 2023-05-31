#include <vector>
#include <WiFi.h> //needed here instead of SysModWebServer as ESPAsyncWebServer.git requires it

// remove latest commit
// git reset --hard HEAD^
// git push origin -f

#include "Module.h"
std::vector<Module *> modules;

#include "SysModPrintServer.h"
#include "SysModFileServer.h"
#include "SysModNetwork.h"
#include "SysModWebServer.h"
#include "SysModUIServer.h"
#include "SysModSystemManager.h"
#include "AppModPinManager.h"

//setup all modules
void setup() {
  print = new SysModPrintServer();
  file = new SysModFileServer();
  net = new SysModNetwork();
  web = new SysModWebServer();
  ui = new SysModUIServer();
  sys = new SysModSystemManager();
  pin = new AppModPinManager();

  modules.push_back(print);
  modules.push_back(file); //90K lps (loops per second)
  modules.push_back(net);
  modules.push_back(web); //80K lps
  modules.push_back(ui);
  modules.push_back(sys);
  modules.push_back(pin); //70K lps -> 110K lps
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