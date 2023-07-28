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
    web->sendDataWs(nullptr, false); //send new data: all clients, no def, no ws here yet!!!
  }

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModModel::setup() {
  Module::setup();

  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentObject = ui->initModule(parentObject, name);

  ui->initText(parentObject, "mSize", nullptr, true, [](JsonObject object) {
    web->addResponse(object["id"], "label", "Size");
  });

  ui->initButton(parentObject, "saveModel", nullptr, [](JsonObject object) {
    web->addResponse(object["id"], "comment", "Write to model.json (manual save only currently)");
  }, [](JsonObject object) {
    doWriteModel = true;
  });

  ui->initCheckBox(parentObject, "showObsolete", false, [](JsonObject object) {
    web->addResponse(object["id"], "comment", "Show in UI (refresh)");
  }, [](JsonObject object) {
    doShowObsolete = object["value"];
  });

  ui->initButton(parentObject, "deleteObsolete", nullptr, [](JsonObject object) {
    web->addResponse(object["id"], "label", "Delete obsolete objects");
    web->addResponse(object["id"], "comment", "WIP");
  }, [](JsonObject object) {
  });

  ui->initButton(parentObject, "deleteModel", nullptr, [](JsonObject object) {
    web->addResponse(object["id"], "comment", "Back to defaults");
  }, [](JsonObject object) {
    print->print("delete model json\n");
    files->remove("/model.json");
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

    // files->writeObjectToFile("/model.json", model);

    JsonRDWS jrdws("/model.json", "w"); //open fileName for deserialize
    jrdws.addExclusion("uiFun");
    jrdws.addExclusion("chFun");
    jrdws.writeJsonDocToFile(model);

    // print->printJson("Write model", *model); //this shows the model before exclusion

    doWriteModel = false;
  }

  if (millis() - secondMillis >= 1000) {
    secondMillis = millis();
    setValueV("mSize", "%u / %u B", model->memoryUsage(), model->capacity());
  }

  if (model->memoryUsage() / model->capacity() > 0.95) {
    print->printJDocInfo("model", *model);
    size_t memBefore = model->memoryUsage();
    model->garbageCollect();
    print->printJDocInfo("garbageCollect", *model);
  }
}

void SysModModel::cleanUpModel(JsonArray objects) {
  for (JsonArray::iterator objectV=objects.begin(); objectV!=objects.end(); ++objectV) {
  // for (JsonVariant objectV : objects) {
    if (objectV->is<JsonObject>()) {
      JsonObject object = objectV->as<JsonObject>();

      //for each object:

      if (object["o"].isNull() || object["o"] >= 0) { //not set negative in initObject
        if (!doShowObsolete)
        //   object["d"] = true;
        // else
          objects.remove(objectV);
      }
      else {
        object["o"] = -object["o"].as<int>(); //make it possitive
      }

      // //for previous not ro values
      // if (object["ro"] && !object["value"].isNull())
      //   object.remove("value");

      //recursive call
      if (!object["n"].isNull() && object["n"].is<JsonArray>())
        cleanUpModel(object["n"]);
    } 
  }
}

//tbd: use template T
//setValue char
JsonObject SysModModel::setValueC(const char * id, const char * value) {
  JsonObject object = findObject(id);
  if (!object.isNull()) {
    if (object["value"].isNull() || object["value"] != value) {
      // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
      if (object["ro"]) { // do not update object["value"]
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
JsonObject SysModModel::setValueI(const char * id, int value) {
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

JsonObject SysModModel::setValueB(const char * id, bool value) {
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
JsonObject SysModModel::setValueV(const char * id, const char * format, ...) {
  va_list args;
  va_start(args, format);

  // size_t len = vprintf(format, args);
  char value[100];
  vsnprintf(value, sizeof(value), format, args);

  va_end(args);

  return setValueC(id, value);
}

JsonObject SysModModel::setValueP(const char * id, const char * format, ...) {
  va_list args;
  va_start(args, format);

  // size_t len = vprintf(format, args);
  char value[100];
  vsnprintf(value, sizeof(value), format, args);
  print->print("%s\n", value);

  va_end(args);

  return setValueC(id, value);
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

JsonObject SysModModel::findObject(const char * id, JsonArray parent) {
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