/*
   @title     StarMod
   @file      UserModMDNS.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include <ESPmDNS.h>

class UserModMDNS:public SysModule {

public:
  String escapedMac;
  char cmDNS[64] = "";

  UserModMDNS() :SysModule("MDNS") {
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    SysModule::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    escapedMac = WiFi.macAddress();
    escapedMac.replace(":", "");
    escapedMac.toLowerCase();

    sprintf(cmDNS, PSTR("wled-%*s"), 6, escapedMac.c_str() + 6);
    // strncpy(cmDNS, "wled-98765", sizeof(cmDNS) -1);

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // SysModule::loop();
  }

  void onOffChanged() {
    if (SysModules::isConnected && isEnabled) {

      // print->fFormat(cmDNS, sizeof(cmDNS)-1, "wled-%*s", WiFi.macAddress().c_str() + 6);

      MDNS.end();
      MDNS.begin(cmDNS);

      USER_PRINTF("mDNS started %s -> %s -> %s\n", WiFi.macAddress().c_str(), escapedMac.c_str(), cmDNS);
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("wled", "tcp", 80);
      MDNS.addServiceTxt("wled", "tcp", "mac", escapedMac.c_str());
    } else {
      MDNS.end();
    }
  }

};

static UserModMDNS *mdns;