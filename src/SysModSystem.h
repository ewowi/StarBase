#include "Module.h"

#include "ArduinoJson.h"

class SysModSystem:public Module {

public:

  SysModSystem();
  void setup();
  void loop();


private:
  unsigned long loopCounter = 0;

  void addResetReasonsLov(JsonArray lov);
  void addRestartReasonsLov(JsonArray lov);

};

static SysModSystem *sys;