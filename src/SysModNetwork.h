#include "Module.h"

#include <DNSServer.h>

class SysModNetwork:public Module {

public:
  bool apActive = false;
  uint32_t lastReconnectAttempt = 0;
  char apSSID[33] = "StarMod AP";
  char apPass[65] = "star1234";
  byte apChannel = 1; // 2.4GHz WiFi AP channel (1-13)
  byte apHide    = 0; // hidden AP SSID
  bool interfacesInited = false;
  static bool forceReconnect;
  DNSServer dnsServer;
  bool noWifiSleep = true;

  SysModNetwork();

  //setup wifi an async webserver
  void setup();

  void loop();

  void handleConnection();

  void initConnection();

  void initAP();

};
  
static SysModNetwork *net;