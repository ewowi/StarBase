class SysModModel:public Module {

public:
  static bool doWriteModel;
  static bool doShowObsolete;

  SysModModel() :Module("Model") {
    JsonArray root = model.to<JsonArray>(); //create

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->println(F("Reading model from /model.json... (deserializeConfigFromFS)"));
    if (readObjectFromFile("/model.json", &model)) {//not part of success...
      print->printJson("Read model", model);
      web->sendDataWs(nullptr, false); //send new data: all clients, no def
    }

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void setup() {
    Module::setup();

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);

    ui->initDisplay(parentObject, "mSize", nullptr, [](JsonObject object) {
      web->addResponse(object, "label", "Size");
    });

    ui->initCheckBox(parentObject, "showObsolete", false, [](JsonObject object) {
      web->addResponse(object, "comment", "Show in UI (refresh)");
    }, [](JsonObject object) {
      doShowObsolete = object["value"];
    });

    ui->initButton(parentObject, "saveModel", "SaveModel", nullptr, [](JsonObject object) {
      doWriteModel = true;
    });
    ui->initButton(parentObject, "deleteObsolete", "DeleteObsolete", [](JsonObject object) {
      web->addResponse(object, "label", "Delete obsolete objects");
    }, [](JsonObject object) {

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
      //clean up model
      cleanUpModel(model.as<JsonArray>());
      print->println(F("Writing model to /model.json... (serializeConfig)"));
      writeObjectToFile("/model.json", &model);
      print->printJson("Write model", model);

      doWriteModel = false;
    }

    if (millis() - secondMillis >= 1000) {
      secondMillis = millis();
      ui->setValueV("mSize", "%u / %u B", model.memoryUsage(), model.capacity());
    }

    if (model.memoryUsage() / model.capacity() > 0.95) {
      print->print("model  %u / %u (%u%%) (%u %u %u)\n", model.memoryUsage(), model.capacity(), 100 * model.memoryUsage() / model.capacity(), model.size(), model.overflowed(), model.nesting());
      size_t memBefore = model.memoryUsage();
      model.garbageCollect();
      print->print("garbageCollect %u / %u%% -> %u / %u%%\n", memBefore, 100 * memBefore / model.capacity(), model.memoryUsage(), 100 * model.memoryUsage() / model.capacity());
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
        print->print("readObjectFromFile deserializeJson failed with code %s\n", error.c_str());
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

bool SysModModel::doWriteModel = false;
bool SysModModel::doShowObsolete = false;
