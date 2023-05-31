#include "Module.h"
#include <DNSServer.h>

//WiFi.h already included in main

class SysModNetwork:public Module {

public:

  const char* clientSSID = "ssid";
  const char* clientPass = "pass";
  bool apActive = false;
  uint32_t lastReconnectAttempt = 0;
  char apSSID[33] = "PlayGround AP";
  char apPass[65] = "play1234";
  byte apChannel = 1; // 2.4GHz WiFi AP channel (1-13)
  byte apHide    = 0; // hidden AP SSID
  bool interfacesInited = false;
  DNSServer dnsServer;
  bool noWifiSleep = true;

  SysModNetwork() :Module("Network Manager") {}; //constructor

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    print->print(" %s\n", success?"success":"failed");
  }

  void loop() {
    // Module::loop();
    handleConnection();
  }

  void handleConnection() {
    if (lastReconnectAttempt == 0) {
      print->print("lastReconnectAttempt == 0\n");
      initConnection();
      return;
    }

    if (!(WiFi.localIP()[0] != 0 && WiFi.status() == WL_CONNECTED)) { //!Network.isConnected()
      if (interfacesInited) {
        print->println(F("Disconnected!"));
        interfacesInited = false;
        initConnection();
      }

      if (!apActive && millis() - lastReconnectAttempt > 12000 ) { //&& (!wasConnected || apBehavior == AP_BEHAVIOR_NO_CONN)
        print->print("Not connected AP.\n");
        initAP();
      }
    } else if (!interfacesInited) { //newly connected
      print->print("Connected! IP address: %s\n", WiFi.localIP().toString().c_str());

      // initInterfaces();
      interfacesInited = true;

      for (Module *module:modules) module->connected();

      // shut down AP
      if (apActive) { //apBehavior != AP_BEHAVIOR_ALWAYS
        // dnsServer.stop();
        WiFi.softAPdisconnect(true);
        apActive = false;
        print->println(F("Access point disabled (handle)."));
      }
    }
  }

  void initConnection() {

    // ws.onEvent(wsEvent);

    WiFi.disconnect(true);        // close old connections

    lastReconnectAttempt = millis();

    if (!apActive) {
      print->println(F("Access point disabled (init)."));
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
    }

    char hostname[25] = "Playground";

    WiFi.setSleep(!noWifiSleep);
    WiFi.setHostname(hostname);

    print->print("Connecting to WiFi %s / ", clientSSID);
    WiFi.begin(clientSSID, clientPass);
    for (int i = 0; i < strlen(clientPass); i++) print->print("*");
    print->print("\n");
  }

  void initAP() {
    print->print("Opening access point %s\n", apSSID);
    WiFi.softAPConfig(IPAddress(4, 3, 2, 1), IPAddress(4, 3, 2, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID, apPass, apChannel, apHide);
    if (!apActive) // start captive portal if AP active
    {
      print->print("Init AP interfaces %s %s %s\n",apSSID, apPass, WiFi.softAPIP().toString().c_str());

      // server.begin();
      for (Module *module:modules) module->connected();

      dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
      dnsServer.start(53, "*", WiFi.softAPIP());
    }
    apActive = true;
  }

  // void initInterfaces() //this should move to seperate UserMods
  // {
  //   interfacesInited = true;
  // }

};
  
static SysModNetwork *net;