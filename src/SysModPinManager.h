#include "Module.h"

#include "ArduinoJson.h"

class SysModPinManager:public Module {

public:

  SysModPinManager();
  void setup();
  void loop();

  void registerPin(uint8_t pinNr);

  static void updateGPIO(JsonObject object);

};

static SysModPinManager *pin;