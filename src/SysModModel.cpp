#include "SysModModel.h"
#include "Module.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModFiles.h"

bool SysModModel::doWriteModel = false;
bool SysModModel::doShowObsolete = false;
DynamicJsonDocument * SysModModel::model = nullptr;

SysModModel::SysModModel() :Module("Model") {
  model = new DynamicJsonDocument(24576);

  JsonArray root = model->to<JsonArray>(); //create

  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  print->println(F("Reading model from /model.json... (deserializeConfigFromFS)"));
  if (files->readObjectFromFile("/model.json", model)) {//not part of success...
    print->printJson("Read model", *model);
    web->sendDataWs(nullptr, false); //send new data: all clients, no def
  }

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModModel::setup() {
  Module::setup();

  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentObject = ui->initGroup(parentObject, name);

  ui->initDisplay(parentObject, "mSize", nullptr, [](JsonObject object) {
    web->addResponse(object, "label", "Size");
  });

  ui->initButton(parentObject, "saveModel", "SaveModel", nullptr, [](JsonObject object) {
    doWriteModel = true;
  });

  ui->initCheckBox(parentObject, "showObsolete", false, [](JsonObject object) {
    web->addResponse(object, "comment", "Show in UI (refresh)");
  }, [](JsonObject object) {
    doShowObsolete = object["value"];
  });

  ui->initButton(parentObject, "deleteObsolete", "DeleteObsolete", [](JsonObject object) {
    web->addResponse(object, "label", "Delete obsolete objects");
  }, [](JsonObject object) {
  });

  ui->initButton(parentObject, "deleteModel", "DeleteModel", nullptr, [](JsonObject object) {
    print->print("delete model json\n");
    files->remove("/model.json");
  });

  ui->initButton(parentObject, "deleteLedMap", "DeleteLedMap", nullptr, [](JsonObject object) {
    print->print("delete ledmap json\n");
    files->remove("/ledmap1.json");
  });

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

  void SysModModel::loop() {
  // Module::loop();

  if (!cleanUpModelDone) { //do after all setups
    cleanUpModelDone = true;
    cleanUpModel(model->as<JsonArray>());
  }
  if (doWriteModel) {
    print->println(F("Writing model to /model.json... (serializeConfig)"));
    files->writeObjectToFile("/model.json", model);
    print->printJson("Write model", *model);

    doWriteModel = false;
  }

  if (millis() - secondMillis >= 1000) {
    secondMillis = millis();
    setValueV("mSize", "%u / %u B", model->memoryUsage(), model->capacity());
  }

  if (model->memoryUsage() / model->capacity() > 0.95) {
    print->print("model  %u / %u (%u%%) (%u %u %u)\n", model->memoryUsage(), model->capacity(), 100 * model->memoryUsage() / model->capacity(), model->size(), model->overflowed(), model->nesting());
    size_t memBefore = model->memoryUsage();
    model->garbageCollect();
    print->print("garbageCollect %u / %u%% -> %u / %u%%\n", memBefore, 100 * memBefore / model->capacity(), model->memoryUsage(), 100 * model->memoryUsage() / model->capacity());
  }
}

void SysModModel::cleanUpModel(JsonArray objects) {
  for (JsonArray::iterator objectV=objects.begin(); objectV!=objects.end(); ++objectV) {
  // for (JsonVariant objectV : objects) {
    if (objectV->is<JsonObject>()) {
      JsonObject object = objectV->as<JsonObject>();

      if (object["o"].isNull() || object["o"] >= 0) { //not set negative in initObject
        if (!doShowObsolete)
        //   object["d"] = true;
        // else
          objects.remove(objectV);
      }
      else {
        object["o"] = -object["o"].as<int>(); //make it possitive
      }

      if (!object["n"].isNull() && object["n"].is<JsonArray>())
        cleanUpModel(object["n"]);
    } 
  }
}

//setValue char
JsonObject SysModModel::setValue(const char * id, const char * value) {
  JsonObject object = findObject(id);
  if (!object.isNull()) {
    if (object["value"].isNull() || object["value"] != value) {
      // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
      if (object["type"] == "display") { // do not update object["value"]
        ui->setChFunAndWs(object, value); //value: bypass object["value"]
      } else {
        object["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
        ui->setChFunAndWs(object);
      }
    }
  }
  else
    print->print("setValue Object %s not found\n", id);
  return object;
}

//setValue int
JsonObject SysModModel::setValue(const char * id, int value) {
  JsonObject object = findObject(id);
  if (!object.isNull()) {
    if (object["value"].isNull() || object["value"] != value) {
      // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
      object["value"] = value;
      ui->setChFunAndWs(object);
    }
  }
  else
    print->print("setValue Object %s not found\n", id);

  return object;
}

JsonObject SysModModel::setValue(const char * id, bool value) {
  JsonObject object = findObject(id);
  if (!object.isNull()) {
    if (object["value"].isNull() || object["value"] != value) {
      // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value?"true":"false");
      object["value"] = value;
      ui->setChFunAndWs(object);
    }
  }
  else
    print->print("setValue Object %s not found\n", id);
  return object;
}

//Set value with argument list
JsonObject SysModModel::setValueV(const char * id, const char * format, ...) { //static to use in *Fun
  va_list args;
  va_start(args, format);

  // size_t len = vprintf(format, args);
  char value[100];
  vsnprintf(value, sizeof(value), format, args);

  va_end(args);

  return setValue(id, value);
}

JsonObject SysModModel::setValueP(const char * id, const char * format, ...) {
  va_list args;
  va_start(args, format);

  // size_t len = vprintf(format, args);
  char value[100];
  vsnprintf(value, sizeof(value), format, args);
  print->print("%s\n", value);

  va_end(args);

  return setValue(id, value);
}

JsonVariant SysModModel::getValue(const char * id) {
  JsonObject object = findObject(id);
  if (!object.isNull())
    return object["value"];
  else {
    print->print("Value of %s does not exist!!\n", id);
    return JsonVariant();
  }
}

JsonObject SysModModel::findObject(const char * id, JsonArray parent) { //static for processJson
  JsonArray root;
  // print ->print("findObject %s %s\n", id, parent.isNull()?"root":"n");
  if (parent.isNull()) {
    root = model->as<JsonArray>();
  }
  else {
    root = parent;
  }
  JsonObject foundObject;
  for(JsonObject object : root) {
    if (foundObject.isNull()) {
      if (object["id"] == id)
        foundObject = object;
      else if (!object["n"].isNull())
        foundObject = findObject(id, object["n"]);
    }
  }
  return foundObject;
}