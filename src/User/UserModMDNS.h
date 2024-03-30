/*
   @title     StarMod
   @file      UserModMDNS.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include <ESPmDNS.h>

class UserModMDNS:public SysModule {

public:
  String escapedMac;
  char cmDNS[64] = ""; //not 33?

  UserModMDNS() :SysModule("MDNS") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    escapedMac = WiFi.macAddress();
    escapedMac.replace(":", "");
    escapedMac.toLowerCase();

    sprintf(cmDNS, PSTR("star-%*s"), 6, escapedMac.c_str() + 6);
    // strncpy(cmDNS, "wled-98765", sizeof(cmDNS) -1);
  }

  void loop() {
    // SysModule::loop();
  }

  void onOffChanged() {
    if (mdls->isConnected && isEnabled) {

      // print->fFormat(cmDNS, sizeof(cmDNS)-1, "wled-%*s", WiFi.macAddress().c_str() + 6);

      MDNS.end();
      MDNS.begin(cmDNS);

      USER_PRINTF("mDNS started %s -> %s -> %s\n", WiFi.macAddress().c_str(), escapedMac.c_str(), cmDNS);
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("star", "tcp", 80);
      MDNS.addServiceTxt("star", "tcp", "mac", escapedMac.c_str());
    } else {
      MDNS.end();
    }
  }

};

extern UserModMDNS *mdns;