#include "Module.h"

#include "ArduinoJson.h"

static DynamicJsonDocument model(24576); //not static as that blows up the stack. Use extern??

//needed to set this here for classes mutually calling other classes (and don't want cpp files ;-)
//they use model and SysModModel uses web and ui...
#include "SysModWeb.h"
#include "SysModUI.h"

class SysModModel:public Module {

public:
  static bool doWriteModel;

  unsigned long dumpMillis = 0;

  SysModModel() :Module("Model") {
    JsonArray root = model.to<JsonArray>(); //create

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->println(F("Reading model from /model.json... (deserializeConfigFromFS)"));
    if (readObjectFromFile("/model.json", &model)) {//not part of success...
      char resStr[200]; 
      serializeJson(model, resStr);
      print->print("Read model %s\n", resStr);
      web->sendDataWs(nullptr, false); //send new data: all clients, no def
    }

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  void setup() {
    Module::setup();

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);

    ui->initDisplay(parentObject, "mSize", nullptr, [](JsonObject object) {
      web->addResponse(object, "label", "Size");
    });

    ui->initButton(parentObject, "saveModel", "SaveModel", nullptr, [](JsonObject object) {
      doWriteModel = true;
    });
    ui->initButton(parentObject, "deleteModel", "DeleteModel", nullptr, [](JsonObject object) {
      print->print("delete model json\n");
      files->remove("/model.json");
    });

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    if (doWriteModel) {
      print->println(F("Writing model to /model.json... (serializeConfig)"));
      writeObjectToFile("/model.json", &model);
      char resStr[200]; 
      serializeJson(model, resStr);
      print->print("write model %s\n", resStr);

      doWriteModel = false;
    }

    if (millis() - secondMillis >= 1000) {
      secondMillis = millis();
      ui->setValueV("mSize", "%u / %u B", model.memoryUsage(), model.capacity());
    }

    if (millis() - dumpMillis >= 60000 || !dumpMillis && model.capacity() / model.memoryUsage() < 2) {
      dumpMillis = millis();
      print->print("model  %u / %u (%u%%) (%u %u %u)\n", model.memoryUsage(), model.capacity(), 100 * model.memoryUsage() / model.capacity(), model.size(), model.overflowed(), model.nesting());
      size_t memBefore = model.memoryUsage();
      model.garbageCollect();
      print->print("garbageCollect %u / %u%% -> %u / %u%%\n", memBefore, 100 * memBefore / model.capacity(), model.memoryUsage(), 100 * model.memoryUsage() / model.capacity());

    }
  }

  bool readObjectFromFile(const char* path, JsonDocument* dest) {
    // if (doCloseFile) closeFile();
    File f = files->open(path, "r");
    if (!f) {
      print->print("File %s open not successful %s\n", path);
      return false;
    }
    else { 
      print->print(PSTR("File %s open to read, size %d bytes\n"), path, (int)f.size());
      DeserializationError error = deserializeJson(*dest, f);
      if (error) {
        print->print("deserializeJson() of definition failed with code %s\n", error.c_str());
        f.close();
        return false;
      } else {
        f.close();
        return true;
      }
    }
  }

  bool writeObjectToFile(const char* path, JsonDocument* dest) {
    File f = files->open(path, "w");
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

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
bool SysModModel::doWriteModel = false;