#include "Module.h"

#include "ArduinoJson.h"
#include <DNSServer.h>

#include <WiFi.h>

class SysModNetwork:public Module {

public:
  bool apActive = false;
  uint32_t lastReconnectAttempt = 0;
  char apSSID[33] = "PlayGround AP";
  char apPass[65] = "play1234";
  byte apChannel = 1; // 2.4GHz WiFi AP channel (1-13)
  byte apHide    = 0; // hidden AP SSID
  bool interfacesInited = false;
  static bool forceReconnect;
  DNSServer dnsServer;
  bool noWifiSleep = true;

  SysModNetwork() :Module("Network") {};

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);
    ui->initInput(parentObject, "SSID", "");
    ui->initPassword(parentObject, "Password", "");
    ui->initButton(parentObject, "Connect",  [](JsonObject object) {
      forceReconnect = true;
    });
    ui->initDisplay(parentObject, "Status");

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
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

    if (forceReconnect) {
      print->print("Forcing reconnect.");
      initConnection();
      interfacesInited = false;
      forceReconnect = false;
      // wasConnected = false;
      return;
    }
    if (!(WiFi.localIP()[0] != 0 && WiFi.status() == WL_CONNECTED)) { //!Network.interfacesInited()
      if (interfacesInited) {
        print->print("Disconnected!\n");
        interfacesInited = false;
        initConnection();
      }

      if (!apActive && millis() - lastReconnectAttempt > 12000 ) { //&& (!wasConnected || apBehavior == AP_BEHAVIOR_NO_CONN)
        print->print("Not connected AP.\n");
        initAP();
      }
    } else if (!interfacesInited) { //newly connected
      ui->setValueP("Status", "Connected %s", WiFi.localIP().toString().c_str());

      interfacesInited = true;

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
      print->print("Access point disabled (init).\n");
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
    }

    WiFi.setSleep(!noWifiSleep);
    WiFi.setHostname("Playground");

    const char* ssid = ui->getValue("SSID");
    const char* password = ui->getValue("Password");
    if (ssid && strlen(ssid)>0) {
      char passXXX [20] = "";
      for (int i = 0; i < strlen(password); i++) strcat(passXXX, "*");
      print->print("Connecting to WiFi %s / %s\n", ssid, passXXX);
      WiFi.begin(ssid, password);
    }
    else
      print->print("No SSID");
  }

  void initAP() {
    print->print("Opening access point %s\n", apSSID);
    WiFi.softAPConfig(IPAddress(4, 3, 2, 1), IPAddress(4, 3, 2, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID, apPass, apChannel, apHide);
    if (!apActive) // start captive portal if AP active
    {
      ui->setValueP("Status", "AP %s / %s @ %s", apSSID, apPass, WiFi.softAPIP().toString().c_str());

      // send all modules connect notification
      for (Module *module:modules) module->connected();

      dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
      dnsServer.start(53, "*", WiFi.softAPIP());
    }
    apActive = true;
  }

};
  
static SysModNetwork *net;

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
bool SysModNetwork::forceReconnect = false;