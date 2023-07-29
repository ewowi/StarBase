#pragma once
// #include "sys/SysModPrint.h"
#include "Module.h"

#include <vector>

class Modules {
  private:
    std::vector<Module *> modules;

  public:
    static bool newConnection; //need to be static otherwise crash

  void setup();

  void loop();

  void add(Module* module);

  void connected();

};

static Modules *mdls;