#include "Module.h"

class SysModSystem:public Module {

public:

  unsigned long lastMillis = 0;
  unsigned long loopCounter = 0;

  SysModSystem() :Module("System") {}; //constructor

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    parentObject = ui->initGroup(JsonObject(), name);

    ui->initDisplay(parentObject, "UpTime");
    ui->initDisplay(parentObject, "LoopPerSecond");
    ui->initDisplay(parentObject, "Heap");
    ui->initDisplay(parentObject, "Stack");

    ui->initButton(parentObject, "Restart", [](const char *prompt, JsonVariant value) {
      ws.closeAll(1012);
      ESP.restart();
    });

    //should be in SysModFiles...
    JsonObject filesObject = ui->initGroup(JsonObject(), "Files");
    ui->initDisplay(filesObject, "Size");
    // ui->initDisplay(filesObject, "Total");

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    loopCounter++;
    if (millis() - lastMillis >= 1000 || !lastMillis) {
      lastMillis = millis();
      ui->setValueV("UpTime", "%u s", millis()/1000);
      ui->setValueV("LoopPerSecond", "%lu", loopCounter);
      ui->setValueV("Heap", "%d / %d (%d) KB", ESP.getFreeHeap()/1000, ESP.getHeapSize()/1000, ESP.getMaxAllocHeap()/1000);
      ui->setValueV("Stack", "%d", uxTaskGetStackHighWaterMark(NULL));

      //should be in SysModFiles...
      ui->setValueV("Size", "%d / %d KB", files->usedBytes()/1000, files->totalBytes()/1000);

      loopCounter = 0;
    }
  }

};

static SysModSystem *sys;