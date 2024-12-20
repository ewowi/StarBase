/*
   @title     StarBase
   @file      UserModMDNS.h
   @date      20241219
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include <ESPmDNS.h>

#include "SysModules.h"

class UserModMDNS:public SysModule {

public:
  String escapedMac;
  char cmDNS[64] = ""; //not 33?

  UserModMDNS() :SysModule("MDNS") {
  };

  void setup() override {
    SysModule::setup();

    escapedMac = WiFi.macAddress();
    escapedMac.replace(":", "");
    escapedMac.toLowerCase();

  }

  void onOffChanged() override {
    if (mdls->isConnected && isEnabled) {

      resetMDNS();

    } else {
      MDNS.end();
    }
  }

  void resetMDNS() {

    if (!mdls->isConnected) return;
    
    //reset cmDNS
    const char * name = mdl->getValue("System", "name");
    ppf("resetMDNS name %s\n", name);
    if (strncmp(name, _INIT(TOSTRING(APP)), 32) == 0 )
      snprintf(cmDNS, sizeof(cmDNS), "star-%*s", 6, escapedMac.c_str() + 6);
    else
      strlcpy(cmDNS, name, sizeof(cmDNS));

    MDNS.end();
    MDNS.begin(cmDNS);

    ppf("mDNS started %s -> %s\n", WiFi.macAddress().c_str(), cmDNS);
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("star", "tcp", 80);
    MDNS.addServiceTxt("star", "tcp", "mac", escapedMac.c_str());
  }

};

extern UserModMDNS *mdns;