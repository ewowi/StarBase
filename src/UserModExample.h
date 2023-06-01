#include "Module.h"

class UserModExample:public Module {

public:

  UserModExample() :Module("Usermod example") {}; //constructor

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

};

static UserModExample *example;