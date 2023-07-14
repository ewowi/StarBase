// remove latest commit
// git reset --hard HEAD^
// git push origin -f

#include "Module.h"
#include "Modules.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModUI.h"
#include "SysModSystem.h"
#include "SysModFiles.h"
#include "SysModModel.h"
#include "SysModNetwork.h"
#include "AppModPinManager.h"
#include "AppModLeds.h"

//setup all modules
void setup() {
  mdls = new Modules();
  print = new SysModPrint();
  files = new SysModFiles();
  mdl = new SysModModel();
  net = new SysModNetwork();
  web = new SysModWeb();
  ui = new SysModUI();
  sys = new SysModSystem();
  pin = new AppModPinManager();
  lds = new AppModLeds();

  mdls->modules.push_back(print);
  mdls->modules.push_back(files);
  mdls->modules.push_back(mdl);
  mdls->modules.push_back(net);
  mdls->modules.push_back(web);
  mdls->modules.push_back(ui);
  mdls->modules.push_back(sys);
  mdls->modules.push_back(pin);
  mdls->modules.push_back(lds);

  mdls->setup();
}

//loop all modules
void loop() {
  mdls->loop();
}