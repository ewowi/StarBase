/*
   @title     StarMod
   @file      main.cpp
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

// remove latest commit
// git reset --hard HEAD^
// git push origin -f

#include "Module.h"
#include "Sys/SysModModules.h"
#include "Sys/SysModPrint.h"
#include "Sys/SysModWeb.h"
#include "Sys/SysModUI.h"
#include "Sys/SysModSystem.h"
#include "Sys/SysModFiles.h"
#include "Sys/SysModModel.h"
#include "Sys/SysModNetwork.h"
#include "Sys/SysModPins.h"
#ifdef APPMOD_LEDS
  #include "App/AppModLeds.h"
  #include "App/AppModLedFixGen.h"
  #ifdef USERMOD_ARTNET
    #include "User/UserModArtNet.h"
  #endif
  #ifdef USERMOD_DDP
    #include "User/UserModDDP.h"
  #endif
#endif
#ifdef USERMOD_E131
  #include "User/UserModE131.h"
#endif
#ifdef USERMOD_HA
  #include "User/UserModHA.h"
#endif
#include "User/UserModWLEDDiscovery.h"

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
  #ifdef APPMOD_LEDS
    lds = new AppModLeds();
    lfg = new AppModLedFixGen();
    #ifdef USERMOD_ARTNET
      artnetmod = new UserModArtNet();
    #endif
    #ifdef USERMOD_DDP
      ddpmod = new UserModDDP();
    #endif
  #endif
  #ifdef USERMOD_E131
    e131mod = new UserModE131();
  #endif
  #ifdef USERMOD_HA
    hamod = new UserModHA();
  #endif
  wledDiscoveryMod = new UserModWLEDDiscovery();

  //prefered default order in the UI
  #ifdef APPMOD_LEDS
    mdls->add(lds);
  #endif
  mdls->add(files);
  mdls->add(sys);
  mdls->add(pin);
  mdls->add(print);
  #ifdef APPMOD_LEDS
    mdls->add(lfg);
    #ifdef USERMOD_ARTNET
      mdls->add(artnetmod);
    #endif
    #ifdef USERMOD_DDP
      mdls->add(ddpmod);
    #endif
  #endif
  mdls->add(ui);
  mdls->add(web);
  mdls->add(net);
  mdls->add(mdl);
  #ifdef USERMOD_E131
    mdls->add(e131mod);
  #endif
  #ifdef USERMOD_HA
    mdls->add(hamod);
  #endif
  mdls->add(wledDiscoveryMod);

  //do not add mdls itself as it does setup and loop for itself!!! (it is the orchestrator)
  mdls->setup();
}

//loop all modules
void loop() {
  mdls->loop();
}