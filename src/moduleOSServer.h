#include "module.h"

class ModuleOSServer:public Module {

public:

  unsigned long lastMillis = 0;
  unsigned long loopCounter = 0;

  ModuleOSServer() :Module("OS Server") {}; //constructor

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    print->print(" %s\n", success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    loopCounter++;
    if (millis() - lastMillis >= 10000) {
      print->print("%lu lps\n", loopCounter/10);
      lastMillis = millis();
      loopCounter = 0;
    }
  }

};

static ModuleOSServer *os;