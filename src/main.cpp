#include <vector>

// remove latest commit
// git reset --hard HEAD^
// git push origin -f

#include "Module.h"
std::vector<Module *> modules;

#include "SysModPrintServer.h"
#include "SysModFiles.h"
#include "SysModModel.h"
#include "SysModNetwork.h"
#include "SysModWebServer.h"
#include "SysModUIServer.h"
#include "SysModSystem.h"
#include "AppModPinManager.h"
#include "AppModLeds.h"

//setup all modules
void setup() {
  print = new SysModPrintServer();
  files = new SysModFiles();
  mdl = new SysModModel();
  net = new SysModNetwork();
  web = new SysModWebServer();
  ui = new SysModUIServer();
  sys = new SysModSystem();
  pin = new AppModPinManager();
  lds = new AppModLeds();

  modules.push_back(print);
  modules.push_back(files); //90K lps (loops per second)
  modules.push_back(mdl);
  modules.push_back(net);
  modules.push_back(web); //80K lps
  modules.push_back(ui);
  modules.push_back(sys);
  modules.push_back(pin); //70K lps -> 110K lps
  modules.push_back(lds);

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