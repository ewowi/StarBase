/*
   @title     StarMod
   @file      SysModModel.cpp
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModModel.h"
#include "Module.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModFiles.h"
#include "SysJsonRDWS.h"

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

  parentVar = ui->initModule(parentVar, name);

  ui->initText(parentVar, "mSize", nullptr, true, [](JsonObject var) {
    web->addResponse(var["id"], "label", "Size");
  });

  ui->initButton(parentVar, "saveModel", nullptr, false, [](JsonObject var) {
    web->addResponse(var["id"], "comment", "Write to model.json (manual save only currently)");
  }, [](JsonObject var) {
    doWriteModel = true;
  });

  ui->initCheckBox(parentVar, "showObsolete", false, false, [](JsonObject var) {
    web->addResponse(var["id"], "comment", "Show in UI (refresh)");
  }, [](JsonObject var) {
    doShowObsolete = var["value"];
  });

  ui->initButton(parentVar, "deleteObsolete", nullptr, false, [](JsonObject var) {
    web->addResponse(var["id"], "label", "Delete obsolete variables");
    web->addResponse(var["id"], "comment", "WIP");
  }, [](JsonObject var) {
  });

  ui->initButton(parentVar, "deleteModel", nullptr, false, [](JsonObject var) {
    web->addResponse(var["id"], "comment", "Back to defaults");
  }, [](JsonObject var) {
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

void SysModModel::cleanUpModel(JsonArray vars) {
  for (JsonArray::iterator varV=vars.begin(); varV!=vars.end(); ++varV) {
  // for (JsonVariant varV : vars) {
    if (varV->is<JsonObject>()) {
      JsonObject var = varV->as<JsonObject>();

      //for each var:

      if (var["o"].isNull() || var["o"] >= 0) { //not set negative in initVar
        if (!doShowObsolete)
        //   var["d"] = true;
        // else
          vars.remove(varV);
      }
      else {
        var["o"] = -var["o"].as<int>(); //make it possitive
      }

      // //for previous not ro values
      // if (var["ro"] && !var["value"].isNull())
      //   var.remove("value");

      //recursive call
      if (!var["n"].isNull() && var["n"].is<JsonArray>())
        cleanUpModel(var["n"]);
    } 
  }
}

//tbd: use template T
//setValue char
JsonObject SysModModel::setValueC(const char * id, const char * value) {
  JsonObject var = findVar(id);
  if (!var.isNull()) {
    if (var["value"].isNull() || var["value"] != value) {
      // print->print("setValue changed %s %s->%s\n", id, var["value"].as<String>().c_str(), value);
      if (var["ro"]) { // do not update var["value"]
        ui->setChFunAndWs(var, value); //value: bypass var["value"]
      } else {
        var["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
        ui->setChFunAndWs(var);
      }
    }
  }
  else
    print->print("setValue Var %s not found\n", id);
  return var;
}

//setValue int
JsonObject SysModModel::setValueI(const char * id, int value) {
  JsonObject var = findVar(id);
  if (!var.isNull()) {
    if (var["value"].isNull() || var["value"] != value) {
      // print->print("setValue changed %s %s->%s\n", id, var["value"].as<String>().c_str(), value);
      var["value"] = value;
      ui->setChFunAndWs(var);
    }
  }
  else
    print->print("setValue Var %s not found\n", id);

  return var;
}

JsonObject SysModModel::setValueB(const char * id, bool value, uint8_t rowNr) {
  JsonObject var = findVar(id);
  if (!var.isNull()) {
    // print->printJson("setValueB", var);
    if (rowNr == (uint8_t)-1) { //normal situation
      if (var["value"].isNull() || var["value"] != value) {
        print->print("setValueB changed %s (%d) %s->%s\n", id, rowNr, var["value"].as<String>().c_str(), value?"true":"false");
        var["value"] = value;
        ui->setChFunAndWs(var);
      }
    }
    else {
      //if we deal with multiple rows, value should be an array, if not we create one

      if (var["value"].isNull() || !var["value"].is<JsonArray>()) {
        print->print("setValueB var %s (%d) value %s not array, creating\n", id, rowNr, var["value"].as<String>().c_str());
        // print->printJson("setValueB var %s value %s not array, creating", id, var["value"].as<String>().c_str());
        var.createNestedArray("value");
      }

      if (var["value"].is<JsonArray>()) {
        //set the right value in the array (if array did not contain values yet, all values before rownr are set to false)
        if (var["value"][rowNr] != value) {
          var["value"][rowNr] = value;
          ui->setChFunAndWs(var);
        }
      }
      else 
        print->print("setValueB %s could not create value array\n", id);
    }
  }
  else
    print->print("setValue Var %s not found\n", id);
  return var;
}

//Set value with argument list
JsonObject SysModModel::setValueV(const char * id, const char * format, ...) {
  va_list args;
  va_start(args, format);

  // size_t len = vprintf(format, args);
  char value[128];
  vsnprintf(value, sizeof(value)-1, format, args);

  va_end(args);

  return setValueC(id, value);
}

JsonObject SysModModel::setValueP(const char * id, const char * format, ...) {
  va_list args;
  va_start(args, format);

  // size_t len = vprintf(format, args);
  char value[128];
  vsnprintf(value, sizeof(value)-1, format, args);
  print->print("%s\n", value);

  va_end(args);

  return setValueC(id, value);
}

JsonVariant SysModModel::getValue(const char * id) {
  JsonObject var = findVar(id);
  if (!var.isNull())
    return var["value"];
  else {
    print->print("Value of Var %s does not exist!!\n", id);
    return JsonVariant();
  }
}

JsonObject SysModModel::findVar(const char * id, JsonArray parent) {
  JsonArray root;
  // print ->print("findVar %s %s\n", id, parent.isNull()?"root":"n");
  if (parent.isNull()) {
    root = model->as<JsonArray>();
  }
  else {
    root = parent;
  }
  JsonObject foundVar;
  for(JsonObject var : root) {
    if (foundVar.isNull()) {
      if (var["id"] == id)
        foundVar = var;
      else if (!var["n"].isNull())
        foundVar = findVar(id, var["n"]);
    }
  }
  return foundVar;
}