/*
   @title     StarMod
   @file      SysModNetwork.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "Module.h"

#include <DNSServer.h>

class SysModNetwork:public Module {

public:

  SysModNetwork();

  //setup wifi an async webserver
  void setup();

  void loop();

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

  static bool forceReconnect;
};
  
static SysModNetwork *net;