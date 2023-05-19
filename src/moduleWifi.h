
#include "module.h"
#include <WiFi.h>

WiFiServer server(80);

class ModuleWifi:public Module {

public:

  ModuleWifi() :Module("Wifi") {};

  void setup() {
    Module::setup();
    Serial.printf("+wifi setup ");
  }

  void loop(){
    Module::loop();
    Serial.printf("+wifi loop ", name);
  }

};