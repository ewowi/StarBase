#pragma once
// #include "SysModFiles.h"
#include "module.h"

#include "ArduinoJson.h"
#include "SysModPrint.h"
#include "SysModFiles.h"

class SysModModel:public Module {

public:
  static bool doWriteModel;
  static bool doShowObsolete;

  // StaticJsonDocument<24576> model; //not static as that blows up the stack. Use extern??
  static DynamicJsonDocument *model;

  SysModModel() :Module("Model") {
    model = new DynamicJsonDocument(24576);

    JsonArray root = model->to<JsonArray>(); //create

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->println(F("Reading model from /model.json... (deserializeConfigFromFS)"));
    if (readObjectFromFile("/model.json", model)) {//not part of success...
      print->printJson("Read model", *model);
      web->sendDataWs(nullptr, false); //send new data: all clients, no def
    }

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void setup();

  void loop(){
    // Module::loop();
    if (doWriteModel) {
      //clean up model
      cleanUpModel(model->as<JsonArray>());
      print->println(F("Writing model to /model.json... (serializeConfig)"));
      writeObjectToFile("/model.json", model);
      print->printJson("Write model", *model);

      doWriteModel = false;
    }

    if (millis() - secondMillis >= 1000) {
      secondMillis = millis();
      ui->setValueV("mSize", "%u / %u B", model->memoryUsage(), model->capacity());
    }

    if (model->memoryUsage() / model->capacity() > 0.95) {
      print->print("model  %u / %u (%u%%) (%u %u %u)\n", model->memoryUsage(), model->capacity(), 100 * model->memoryUsage() / model->capacity(), model->size(), model->overflowed(), model->nesting());
      size_t memBefore = model->memoryUsage();
      model->garbageCollect();
      print->print("garbageCollect %u / %u%% -> %u / %u%%\n", memBefore, 100 * memBefore / model->capacity(), model->memoryUsage(), 100 * model->memoryUsage() / model->capacity());
    }
  }

  void cleanUpModel(JsonArray objects) {
    for (JsonArray::iterator objectV=objects.begin(); objectV!=objects.end(); ++objectV) {
    // for (JsonVariant objectV : objects) {
      if (objectV->is<JsonObject>()) {

        JsonObject object = objectV->as<JsonObject>();

        if ((object)["type"] == "display")  {
          // objects.remove(objectV); //not for now as app needs it (rebuild needed?)
        }
        else {
          for (JsonPair pair : object) { //iterate json elements
            JsonVariant value = pair.value();
          }

          object.remove("s");

          if (!object["n"].isNull() && object["n"].is<JsonArray>())
            cleanUpModel(object["n"]);
        }
      } 
    }
  }

  bool readObjectFromFile(const char* path, JsonDocument* dest);

  bool writeObjectToFile(const char* path, JsonDocument* dest);

};

static SysModModel *mdl;

