/*
   @title     StarMod
   @file      SysModNetwork.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

#include <DNSServer.h>

class SysModNetwork:public SysModule {

public:

  SysModNetwork();

  //setup wifi an async webserver
  void setup();

  void loop();
  void loop1s();

  void handleConnection();

  void initConnection();

  void initAP();
  
private:
  bool apActive = false;
  uint32_t lastReconnectAttempt = 0;
  char apSSID[33] = "StarMod AP";
  char apPass[65] = "star1234";
  byte apChannel = 1; // 2.4GHz WiFi AP channel (1-13)
  byte apHide    = 0; // hidden AP SSID
  bool interfacesInited = false;
  DNSServer dnsServer;
  bool noWifiSleep = true;

  //init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
  bool forceReconnect = false;
};
  
static SysModNetwork *net;