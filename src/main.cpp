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
#include "SysModPinManager.h"
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
  pin = new SysModPinManager();
  lds = new AppModLeds();

  //prefered default order in the UI
  mdls->add(lds);
  mdls->add(files);
  mdls->add(sys);
  mdls->add(pin);
  mdls->add(print);
  mdls->add(mdl);
  mdls->add(ui);
  mdls->add(web);
  mdls->add(net);

  mdls->setup();
}

//loop all modules
void loop() {
  mdls->loop();
  // delay(1);//feed the watchdog
}