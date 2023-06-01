// #pragma once
#include "Module.h"

#include "ArduinoJson.h"

DynamicJsonDocument model(10240); //not static as that blows up the stack

//needed to set this here for classes mutually calling other classes (and don't want cpp files ;-)
#include "SysModWebServer.h"
#include "SysModUIServer.h"

//try this !!!: curl -X POST "http://192.168.121.196/json" -d '{"Pin2":false}' -H "Content-Type: application/json"

static bool doWriteModel = false;

class SysModModel:public Module {

public:
  unsigned long dumpMillis = 0;
  unsigned long secondMillis = 0;

  SysModModel() :Module("Model") {}; //constructor

  //setup filesystem
  void setup() {
    Module::setup();

    print->print("%s Setup:", name);

    JsonArray root = model.to<JsonArray>(); //create

    print->println(F("Reading model from /model.json... (deserializeConfigFromFS)"));
    if (readObjectFromFile("/model.json", &model)) {//not part of success...
      serializeJson(model, Serial);
      web->sendDataWs(nullptr, false); //send new data
    }

    ui->defButton("DeleteModel", "DeleteModel", [](const char *prompt, JsonVariant value) {
      print->print("delete model json\n");
      file->remove("/model.json");
    });
    ui->defButton("SaveModel", "SaveModel", [](const char *prompt, JsonVariant value) {
      doWriteModel = true;
    });

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    if (doWriteModel) {
      print->println(F("Writing model to /model.json... (serializeConfig)"));
      writeObjectToFile("/model.json", &model);
      serializeJson(model, Serial);

      doWriteModel = false;
    }

    if (millis() - secondMillis >= 1000) {
      secondMillis = millis();
      char nS[32];
      ui->defGroup(name);
      ui->setDisplay("memoryUsage", itoa(model.memoryUsage(), nS, 10));
      ui->setDisplay("capacity", itoa(model.capacity(), nS, 10));
    }

    if (millis() - dumpMillis >= 60000 || !dumpMillis && model.capacity() / model.memoryUsage() < 2) {
      dumpMillis = millis();
      // serializeJsonPretty(model, Serial);
      print->print("model  %u / %u (%u%%) (%u %u %u)\n", model.memoryUsage(), model.capacity(), 100 * model.memoryUsage() / model.capacity(), model.size(), model.overflowed(), model.nesting());
      size_t memBefore = model.memoryUsage();
      model.garbageCollect();
      print->print("garbageCollect %u / %u%% -> %u / %u%%\n", memBefore, 100 * memBefore / model.capacity(), model.memoryUsage(), 100 * model.memoryUsage() / model.capacity());

    }
  }

  bool readObjectFromFile(const char* path, JsonDocument* dest) {
    // if (doCloseFile) closeFile();
    File f = file->open(path, "r");
    if (!f) {
      print->print("File %s open not successful %s", path);
      return false;
    }
    else { 
      print->print(PSTR("File %s open to read, size %d bytes\n"), path, (int)f.size());
      deserializeJson(*dest, f);
      f.close();
      return true;
    }
  }

  bool writeObjectToFile(const char* path, JsonDocument* dest) {
    File f = file->open(path, "w");
    if (f) {
      print->println(F("  success"));
      serializeJson(*dest, f);
      return true;
    } else {
      f.close();
      print->println(F("  fail"));
      return false;
    }
  }

};

static SysModModel *mdl;