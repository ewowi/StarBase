#include "module.h"

class ModuleUI:public Module {

public:

  ModuleUI() :Module("UIServer") {}; //constructor

  //serve index.htm
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    success = success && web->addURL("/", "/index.htm", "text/html");

    print->print(" %s\n", success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

};

ModuleUI *ui;