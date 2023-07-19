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
#include "UserModE131.h"

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
  e131mod = new UserModE131();

  mdls->add(print);
  mdls->add(files);
  mdls->add(mdl);
  mdls->add(net);
  mdls->add(web);
  mdls->add(ui);
  mdls->add(sys);
  mdls->add(pin);
  mdls->add(lds);
  mdls->add(e131mod);

  mdls->setup();
}

//loop all modules
void loop() {
  mdls->loop();
}