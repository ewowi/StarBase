#include "Module.h"

#include "ArduinoJson.h"
#include <DNSServer.h>

#include <WiFi.h>

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
  bool isConnected = false; //aka interfacesInited
  DNSServer dnsServer;
  bool noWifiSleep = true;

  SysModNetwork() :Module("Network") {}; //constructor

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    ui->initGroup(name);
    ui->initInput("clientSSID", "ssid");
    ui->initInput("clientPass", "pass");

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();
    handleConnection();
  }

  void handleConnection() {
    if (lastReconnectAttempt == 0) { // do this only once
      print->print("lastReconnectAttempt == 0\n");
      initConnection();
      return;
    }

    if (!(WiFi.localIP()[0] != 0 && WiFi.status() == WL_CONNECTED)) { //!Network.isConnected()
      if (isConnected) {
        print->println(F("Disconnected!"));
        isConnected = false;
        initConnection();
      }

      if (!apActive && millis() - lastReconnectAttempt > 12000 ) { //&& (!wasConnected || apBehavior == AP_BEHAVIOR_NO_CONN)
        print->print("Not connected AP.\n");
        initAP();
      }
    } else if (!isConnected) { //newly connected
      print->print("Connected! IP address: %s\n", WiFi.localIP().toString().c_str());

      isConnected = true;

      for (Module *module:modules) module->connected();

      // shut down AP
      if (apActive) { //apBehavior != AP_BEHAVIOR_ALWAYS
        dnsServer.stop();
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

    WiFi.setSleep(!noWifiSleep);
    WiFi.setHostname("Playground");

    const char * test = ui->getValue("clientSSID");
    print->print("Connecting to WiFi %s (%s) / ", clientSSID, test?test:"");
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

};
  
static SysModNetwork *net;