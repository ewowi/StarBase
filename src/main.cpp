/*
   @title     StarMod
   @file      main.cpp
   @date      20240411
   @repo      https://github.com/ewowi/StarMod, submit changes to this file as PRs to ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"
#include "SysModules.h"
#include "Sys/SysModPrint.h"
#include "Sys/SysModWeb.h"
#include "Sys/SysModUI.h"
#include "Sys/SysModSystem.h"
#include "Sys/SysModFiles.h"
#include "Sys/SysModModel.h"
#include "Sys/SysModNetwork.h"
#include "Sys/SysModPins.h"
#include "User/UserModInstances.h"
#include "User/UserModMDNS.h"
SysModules *mdls;
SysModPrint *print;
SysModWeb *web;
SysModUI *ui;
SysModSystem *sys;
SysModFiles *files;
SysModModel *mdl;
SysModNetwork *net;
SysModPins *pins;
UserModInstances *instances;
UserModMDNS *mdns;

#include "App/AppModDemo.h"
AppModDemo *appModDemo;

#ifdef STARMOD_USERMOD_E131
  #include "User/UserModE131.h"
  UserModE131 *e131mod;
#endif
#ifdef STARMOD_USERMOD_HA
  #include "User/UserModHA.h"
  UserModHA *hamod;
#endif

//setup all modules
void setup() {
  mdls = new SysModules();
  
  print = new SysModPrint();
  files = new SysModFiles();
  mdl = new SysModModel();
  net = new SysModNetwork();
  web = new SysModWeb();
  ui = new SysModUI();
  sys = new SysModSystem();
  pins = new SysModPins();
  instances = new UserModInstances();
  mdns = new UserModMDNS();
  appModDemo = new AppModDemo();
  #ifdef STARMOD_USERMOD_E131
    e131mod = new UserModE131();
  #endif
  #ifdef STARMOD_USERMOD_HA
    hamod = new UserModHA();
  #endif

  //Reorder with care! this is the order in which setup and loop is executed
  //If changed make sure mdlEnabled.chFun executes var["value"].to<JsonArray>(); and saveModel! 
  //Default: add below, not in between

  mdls->add(appModDemo);

  mdls->add(files);
  mdls->add(sys);
  mdls->add(pins);
  mdls->add(print);
  mdls->add(web);
  mdls->add(net);
  #ifdef STARMOD_USERMOD_E131
    mdls->add(e131mod);
  #endif
  #ifdef STARMOD_USERMOD_HA
    mdls->add(hamod); //no ui
  #endif
  mdls->add(mdl);
  mdls->add(ui);
  mdls->add(mdns); //no ui
  mdls->add(instances);

  //do not add mdls itself as it does setup and loop for itself!!! (it is the orchestrator)
  mdls->setup();
}

//loop all modules
void loop() {
  mdls->loop();
}