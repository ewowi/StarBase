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
      print->print("%lu / %lu = %lu lps\n", loopCounter, millis()/1000, 1000 * loopCounter / millis());
      lastMillis = millis();
      loopCounter = 0;
    }
  }

};

static ModuleOSServer *os;