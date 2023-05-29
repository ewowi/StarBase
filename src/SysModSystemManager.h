#include "Module.h"

class SysModSystemManager:public Module {

public:

  unsigned long lastMillis = 0;
  unsigned long loopCounter = 0;

  SysModSystemManager() :Module("System") {}; //constructor

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    ui->addGroup(name);
    ui->addDisplay("LoopPerSecond", "");
    ui->addDisplay("UpTime", "");
    ui->addDisplay("TotalHeap", "");
    ui->addDisplay("FreeHeap", "");

    print->print(" %s\n", success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    loopCounter++;
    if (millis() - lastMillis >= 1000 || !lastMillis) {
      lastMillis = millis();
      char nS[32];
      ui->setInput("LoopPerSecond", itoa(loopCounter, nS, 10));
      ui->setInput("UpTime", itoa(millis()/1000, nS, 10));
      ui->setInput("TotalHeap", itoa(ESP.getHeapSize(), nS, 10));
      ui->setInput("FreeHeap", itoa(ESP.getFreeHeap(), nS, 10));
      loopCounter = 0;
    }
  }

};

static SysModSystemManager *sys;