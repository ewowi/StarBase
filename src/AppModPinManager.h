#include "Module.h"

//try this !!!: curl -X POST "http://192.168.121.196/json" -d '{"Pin2":false}' -H "Content-Type: application/json"

class AppModPinManager:public Module {

public:

  AppModPinManager() :Module("Pin Manager") {}; //constructor

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    pinMode(4, OUTPUT);
    pinMode(33, OUTPUT);

    ui->addGroup(name);
    ui->addCheckBox("Pin2", true, updateGPIO);
    ui->addCheckBox("Pin4", false);
    ui->addCheckBox("Pin33", true);

    ui->finishUI();
  
    print->print(" %s\n", success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

  static void updateGPIO(JsonPair pair) {
    if (pair.value().is<bool>()) {
      bool value = pair.value().as<bool>();
      const char * key = pair.key().c_str();

      print->print("updateGPIO %s:=%d\n", key, value);

      if (strcmp(key, "Pin2") == 0) digitalWrite(2, value?HIGH:LOW);
      if (strcmp(key, "Pin4") == 0) digitalWrite(4, value?HIGH:LOW);
      if (strcmp(key, "Pin33") == 0) digitalWrite(33, value?HIGH:LOW);
    }
  }

};

static AppModPinManager *pin;