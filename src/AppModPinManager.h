#include "Module.h"

class AppModPinManager:public Module {

public:

  AppModPinManager() :Module("Pin Manager") {};

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    pinMode(2, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(33, OUTPUT);

    parentObject = ui->initGroup(parentObject, name);
    ui->initCheckBox(parentObject, "Pin2", true, updateGPIO);
    ui->initCheckBox(parentObject, "Pin4", false);
    ui->initCheckBox(parentObject, "Pin33", true);

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

  static void updateGPIO(JsonObject object) {
    if (object["value"].is<bool>()) {
      bool pin = object["value"].as<bool>();

      print->print("updateGPIO %s:=%d\n", object["prompt"], pin);

      if (strcmp(object["prompt"], "Pin2") == 0) digitalWrite(2, pin?HIGH:LOW);
      if (strcmp(object["prompt"], "Pin4") == 0) digitalWrite(4, pin?HIGH:LOW);
      if (strcmp(object["prompt"], "Pin33") == 0) digitalWrite(33, pin?HIGH:LOW);
    }
  }

};

static AppModPinManager *pin;