#include "Module.h"

#include "ArduinoJson.h"

class SysModSystem:public Module {

public:

  SysModSystem();
  void setup();
  void loop();


private:
  unsigned long loopCounter = 0;

  void addResetReasonsSelect(JsonArray select);
  void addRestartReasonsSelect(JsonArray select);

};

static SysModSystem *sys;