#include "module.h"

#include "ArduinoJson.h"

class SysModSystem:public Module {

public:

  unsigned long loopCounter = 0;

  SysModSystem();
  void setup();
  void loop();

  void addResetReasonsLov(JsonArray lov);

};

static SysModSystem *sys;