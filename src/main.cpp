// remove latest commit
// git reset --hard HEAD^
// git push origin -f

#include "Module.h"
#include "Modules.h"
#include "sys/SysModPrint.h"
#include "sys/SysModWeb.h"
#include "sys/SysModUI.h"
#include "sys/SysModSystem.h"
#include "sys/SysModFiles.h"
#include "sys/SysModModel.h"
#include "sys/SysModNetwork.h"
#include "sys/SysModPins.h"
#include "app/AppModLeds.h"
#include "app/AppModLedFixGen.h"

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
  pin = new SysModPins();
  lds = new AppModLeds();
  lfg = new AppModLedFixGen();

  //prefered default order in the UI
  mdls->add(lds);
  mdls->add(files);
  mdls->add(sys);
  mdls->add(pin);
  mdls->add(print);
  mdls->add(lfg);
  mdls->add(ui);
  mdls->add(web);
  mdls->add(net);
  mdls->add(mdl);

  mdls->setup();
}

//loop all modules
void loop() {
  mdls->loop();
}