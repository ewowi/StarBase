#include "Module.h"

class AppModPinManager:public Module {

public:

  AppModPinManager() :Module("Pin Manager") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    pinMode(2, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(33, OUTPUT);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);
    ui->initCheckBox(parentObject, "Pin2", true, updateGPIO);
    ui->initCheckBox(parentObject, "Pin4", false);
    ui->initCheckBox(parentObject, "Pin33", true);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

  static void updateGPIO(JsonObject object) {
    if (object["value"].is<bool>()) {
      bool pin = object["value"];
      const char *prompt = object["prompt"];

      print->print("updateGPIO %s:=%d\n", prompt, pin);

      if (strcmp(prompt, "Pin2") == 0) digitalWrite(2, pin?HIGH:LOW);
      if (strcmp(prompt, "Pin4") == 0) digitalWrite(4, pin?HIGH:LOW);
      if (strcmp(prompt, "Pin33") == 0) digitalWrite(33, pin?HIGH:LOW);
    }
  }

};

static AppModPinManager *pin;