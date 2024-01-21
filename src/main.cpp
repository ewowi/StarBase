/*
   @title     StarMod
   @file      main.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

// remove latest commit
// git reset --hard HEAD^
// git push origin -f

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
#ifdef APPMOD_LEDS
  #include "App/AppModLeds.h"
  #include "App/AppModPreview.h"
  #include "App/AppModFixtureGen.h"
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
#ifdef USERMOD_WLEDAUDIO
  #include "User/UserModWLEDAudio.h"
#endif

//setup all modules
void setup() {
  // pinMode(LED_BUILTIN, OUTPUT);
  // digitalWrite(LED_BUILTIN, HIGH);

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
  #ifdef APPMOD_LEDS
    lds = new AppModLeds();
    pvw = new AppModPreview();
    lfg = new AppModFixtureGen();
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
  #ifdef USERMOD_WLEDAUDIO
    wledAudioMod = new UserModWLEDAudio();
  #endif

  //preferred default order in the UI. 
  //Reorder with care! If changed make sure mdlEnabled.chFun executes var.createNestedArray("value"); and saveModel! 
  //Default: add below, not in between
  #ifdef APPMOD_LEDS
    mdls->add(lds);
    mdls->add(pvw);
    mdls->add(lfg);
  #endif
  mdls->add(files);
  mdls->add(sys);
  mdls->add(pins);
  mdls->add(print);
  mdls->add(web);
  mdls->add(net);
  #ifdef APPMOD_LEDS
    #ifdef USERMOD_DDP
      mdls->add(ddpmod);
    #endif
    #ifdef USERMOD_ARTNET
      mdls->add(artnetmod);
    #endif
  #endif
  #ifdef USERMOD_E131
    mdls->add(e131mod);
  #endif
  #ifdef USERMOD_HA
    mdls->add(hamod);
  #endif
  mdls->add(mdl);
  mdls->add(ui);
  #ifdef USERMOD_WLEDAUDIO
    mdls->add(wledAudioMod);
  #endif
  mdls->add(mdns); //no ui
  mdls->add(instances);

  //do not add mdls itself as it does setup and loop for itself!!! (it is the orchestrator)
  mdls->setup();

  // digitalWrite(LED_BUILTIN, LOW);
}

//loop all modules
void loop() {
  mdls->loop();

  // static bool onoff = false;
  // onoff = !onoff;
  // digitalWrite(LED_BUILTIN, onoff? HIGH:LOW);
}