#include "Module.h"

//WiFi.h already included in main

class SysModNetwork:public Module {

public:

  const char* ssid = "ssid";
  const char* pass = "pass";

  SysModNetwork() :Module("Wifi Manager") {}; //constructor

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    print->print(" Connecting to WiFi %s / ", ssid);
    WiFi.begin(ssid, pass);
    for (int i = 0; i < strlen(pass); i++) print->print("*");
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      print->print(".");
    }
    print->print("!");
 
    print->print(" IP %s", WiFi.localIP().toString().c_str());

    print->print(" %s\n", success?"success":"failed");
  }

  void loop() {
    // Module::loop();
    handleConnection();
  }

  void handleConnection()
  {
  }

};

static SysModNetwork *net;