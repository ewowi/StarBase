#include "module.h"

class ModuleToggle:public Module {

public:

  bool p2 = false;
  bool p4 = true;
  bool p33 = false;

  ModuleToggle() :Module("UIServerToggle") {}; //constructor

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    pinMode(2, OUTPUT);
    digitalWrite(2, p2?HIGH:LOW);
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    pinMode(33, OUTPUT);
    digitalWrite(33, LOW);

  
    ui->addCheckBox("Pin2", &p2, updateGPIO);
    ui->addCheckBox("Pin4", &p4);
    ui->addCheckBox("Pin33", &p33);

    ui->finishUI();
      
    print->print(" %s\n", success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    p2 = random(2);
    p4 = random(2);
    p33 = random(2);
  }

  static void updateGPIO(String id, String value) {
    print->print("updateGPIO %s:=%s\n", id, value);
    if (id == "Pin2") digitalWrite(2, value=="true"?HIGH:LOW);
    if (id == "Pin4") digitalWrite(4, value=="true"?HIGH:LOW);
    if (id == "Pin33") digitalWrite(33, value=="true"?HIGH:LOW);
  }

};

static ModuleToggle *toggle;