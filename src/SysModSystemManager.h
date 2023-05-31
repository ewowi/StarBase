#include "Module.h"

class SysModSystemManager:public Module {

public:

  unsigned long lastMillis = 0;
  unsigned long loopCounter = 0;

  SysModSystemManager() :Module("System") {}; //constructor

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    ui->defGroup(name);

    ui->defDisplay("LoopPerSecond");
    ui->defDisplay("UpTime");
    ui->defDisplay("TotalHeap");
    ui->defDisplay("FreeHeap");
    ui->defDisplay("usedBytes");
    ui->defDisplay("totalBytes");

    ui->defButton("DeleteModel", "DeleteModel", [](const char *prompt, JsonVariant value) {
      print->print("delete model json\n");
      LittleFS.remove("/cfg.json");
    });
    ui->defButton("SaveModel", "SaveModel", [](const char *prompt, JsonVariant value) {
      ui->doWriteModel = true;
    });
    ui->defButton("Restart", "Restart", [](const char *prompt, JsonVariant value) {
      ws.closeAll(1012);
      ESP.restart();
    });


    print->print(" %s\n", success?"success":"failed");
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
      ui->setDisplay("FreeHeap", itoa(ESP.getFreeHeap(), nS, 10));
      ui->setDisplay("usedBytes", itoa(LittleFS.usedBytes(), nS, 10));
      ui->setDisplay("totalBytes", itoa(LittleFS.totalBytes(), nS, 10));

      loopCounter = 0;
    }
  }

};

static SysModSystemManager *sys;