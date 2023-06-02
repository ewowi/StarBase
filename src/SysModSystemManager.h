#include "Module.h"

class SysModSystemManager:public Module {

public:

  unsigned long lastMillis = 0;
  unsigned long loopCounter = 0;

  SysModSystemManager() :Module("System") {}; //constructor

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    // ui->initDisplay("LoopPerSecond");
    // ui->initDisplay("UpTime");
    // ui->initDisplay("TotalHeap");
    // ui->initDisplay("FreeHeap");
    // ui->initDisplay("usedBytes");
    // ui->initDisplay("totalBytes");

    ui->initGroup(name);
    ui->initButton("Restart", "Restart", [](const char *prompt, JsonVariant value) {
      ws.closeAll(1012);
      ESP.restart();
    });


    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();
    // now = millis();

    loopCounter++;
    if (millis() - lastMillis >= 1000 || !lastMillis) {
      lastMillis = millis();
      char nS[32];

      ui->setDisplay("LoopPerSecond", itoa(loopCounter, nS, 10));
      ui->setDisplay("UpTime", itoa(millis()/1000, nS, 10));
      ui->setDisplay("TotalHeap", itoa(ESP.getHeapSize(), nS, 10));
      ui->setDisplay("MaxAlloc", itoa(ESP.getMaxAllocHeap(), nS, 10));
      ui->setDisplay("FreeHeap", itoa(ESP.getFreeHeap(), nS, 10));
      ui->setDisplay("usedBytes", itoa(file->usedBytes(), nS, 10));
      ui->setDisplay("totalBytes", itoa(file->totalBytes(), nS, 10));
      ui->setDisplay("minFreeStack", itoa(uxTaskGetStackHighWaterMark(NULL), nS, 10));

      loopCounter = 0;
    }
  }

};

static SysModSystemManager *sys;