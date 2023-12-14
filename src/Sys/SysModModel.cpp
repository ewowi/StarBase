/*
   @title     StarMod
   @file      SysModModel.cpp
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModModel.h"
#include "SysModule.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModFiles.h"
#include "SysJsonRDWS.h"

DynamicJsonDocument * SysModModel::model = nullptr;
JsonObject SysModModel::modelParentVar;

SysModModel::SysModModel() :SysModule("Model") {
  model = new DynamicJsonDocument(24576);

  JsonArray root = model->to<JsonArray>(); //create

  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  USER_PRINTF("Reading model from /model.json... (deserializeConfigFromFS)\n");
  if (files->readObjectFromFile("/model.json", model)) {//not part of success...
    print->printJson("Read model", *model);
    web->sendDataWs(*model);
  } else {
    root = model->to<JsonArray>(); //re create the model as it is corrupted by readFromFile
  }

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModModel::setup() {
  SysModule::setup();

  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = ui->initModule(parentVar, name);

  ui->initText(parentVar, "mSize", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Size");
  });

  ui->initButton(parentVar, "saveModel", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Write to model.json (manual save only currently)");
  }, [this](JsonObject var, uint8_t) { //chFun
    doWriteModel = true;
  });

  ui->initCheckBox(parentVar, "showObsolete", doShowObsolete, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Show in UI (refresh)");
  }, [this](JsonObject var, uint8_t) { //chFun
    doShowObsolete = var["value"];
  });

  ui->initButton(parentVar, "deleteObsolete", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Delete obsolete variables");
    web->addResponse(var["id"], "comment", "WIP");
  });

  ui->initButton(parentVar, "deleteModel", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Back to defaults");
  }, [](JsonObject var, uint8_t) { //chFun
    USER_PRINTF("delete model json\n");
    files->remove("/model.json");
  });

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

  void SysModModel::loop() {
  // SysModule::loop();

  if (!cleanUpModelDone) { //do after all setups
    cleanUpModelDone = true;
    cleanUpModel(model->as<JsonArray>());
  }

  if (doWriteModel) {
    USER_PRINTF("Writing model to /model.json... (serializeConfig)\n");

    // files->writeObjectToFile("/model.json", model);

    cleanUpModel(model->as<JsonArray>(), false, true);//remove if var["o"] is negative (not cleanedUp) and remove ro values

    JsonRDWS jrdws("/model.json", "w"); //open fileName for deserialize
    jrdws.addExclusion("uiFun");
    jrdws.addExclusion("chFun");
    jrdws.writeJsonDocToFile(model);

    // print->printJson("Write model", *model); //this shows the model before exclusion

    doWriteModel = false;
  }

  if (model->memoryUsage() / model->capacity() > 0.95) {
    print->printJDocInfo("model", *model);
    size_t memBefore = model->memoryUsage();
    model->garbageCollect();
    print->printJDocInfo("garbageCollect", *model);
  }
}
void SysModModel::loop1s() {
  setValueLossy("mSize", "%d / %d B", model->memoryUsage(), model->capacity());
}

void SysModModel::cleanUpModel(JsonArray vars, bool oPos, bool ro) {
  for (JsonArray::iterator varV=vars.begin(); varV!=vars.end(); ++varV) {
  // for (JsonVariant varV : vars) {
    if (varV->is<JsonObject>()) {
      JsonObject var = varV->as<JsonObject>();

      //no cleanup of o in case of ro value removal
      if (!ro) {
        if (oPos) {
          if (var["o"].isNull() || var["o"].as<int>() >= 0) { //not set negative in initVar
            if (!doShowObsolete) {
              USER_PRINTF("remove var %s (o>=0)\n", var["id"].as<const char *>());          
              vars.remove(varV); //remove the obsolete var (no o or )
            }
          }
          else {
            var["o"] = -var["o"].as<int>(); //make it possitive
          }
        } else { //!oPos
          if (var["o"].isNull() || var["o"].as<int>() < 0) { 
            USER_PRINTF("remove var %s (o<0)\n", var["id"].as<const char *>());          
            vars.remove(varV); //remove the obsolete var (no o or o is negative - not cleanedUp)
          }
        }
      }

      //remove ro values (ro vars cannot be deleted as SM uses these vars)
      if (ro && var["ro"].as<bool>()) {// && !var["value"].isNull())
        USER_PRINTF("remove ro value %s\n", var["id"].as<const char *>());          
        // vars.remove(varV); //remove ro vars
        var.remove("value");
      }

      //recursive call
      if (!var["n"].isNull() && var["n"].is<JsonArray>())
        cleanUpModel(var["n"], oPos, ro);
    } 
  }
}

//tbd: use template T
//setValue char
JsonObject SysModModel::setValueC(const char * id, const char * value, uint8_t rowNr) {
  JsonObject var = findVar(id);
  if (!var.isNull()) {
    if (rowNr == uint8Max) { //normal situation
      if (var["value"].isNull() || var["value"] != value) {
        // USER_PRINTF("setValue changed %s %s->%s\n", id, var["value"].as<String>().c_str(), value);
        if (var["ro"]) { // do not update var["value"]
          ui->setChFunAndWs(var, rowNr, value); //value: bypass var["value"]
          //now only used for ro not lossy
          USER_PRINTF("setValueC: RO non lossy %s %s\n", id, value);
        } else {
          var["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
          ui->setChFunAndWs(var, rowNr);
        }
      }
    } else {
      //if we deal with multiple rows, value should be an array, if not we create one

      if (var["value"].isNull() || !var["value"].is<JsonArray>()) {
        USER_PRINTF("setValueC var %s (%d) value %s not array, creating\n", id, rowNr, var["value"].as<String>().c_str());
        // print->printJson("setValueB var %s value %s not array, creating", id, var["value"].as<String>().c_str());
        var.createNestedArray("value");
      }

      if (var["value"].is<JsonArray>()) {
        //set the right value in the array (if array did not contain values yet, all values before rownr are set to false)
        if (var["value"][rowNr] != value) {
          var["value"][rowNr] = (char *)value; //create a copy
          ui->setChFunAndWs(var, rowNr);
        }
      }
      else 
        USER_PRINTF("setValueI %s could not create value array\n", id);
      // USER_PRINTF("setValueC Var %s (%s) table row nr not implemented yet %d\n", id, value, rowNr);
    }
  }
  else
    USER_PRINTF("setValueC Var %s not found\n", id);
  return var;
}

//setValue int
JsonObject SysModModel::setValueI(const char * id, int value, uint8_t rowNr) {
  JsonObject var = findVar(id);
  if (!var.isNull()) {
    if (rowNr == uint8Max) { //normal situation
      if (var["value"].isNull() || var["value"] != value) {
        // USER_PRINTF("setValue changed %s %s->%s\n", id, var["value"].as<String>().c_str(), value);
        var["value"] = value;
        ui->setChFunAndWs(var, rowNr);
      }
    }
    else {
      //if we deal with multiple rows, value should be an array, if not we create one

      if (var["value"].isNull() || !var["value"].is<JsonArray>()) {
        USER_PRINTF("setValueI var %s value %s not array, creating\n", id, var["value"].as<String>().c_str());
        // print->printJson("setValueB var %s value %s not array, creating", id, var["value"].as<String>().c_str());
        var.createNestedArray("value");
      }

      if (var["value"].is<JsonArray>()) {
        //set the right value in the array (if array did not contain values yet, all values before rownr are set to false)
        if (var["value"][rowNr] != value) {
          USER_PRINTF("setValueI var %s[%d] value %s := %d\n", id, rowNr, var["value"].as<String>().c_str(), value);
          var["value"][rowNr] = value;
          ui->setChFunAndWs(var, rowNr);
        }
      }
      else 
        USER_PRINTF("setValueI %s could not create value array\n", id);
    }
  }
  else
    USER_PRINTF("setValueI Var %s not found\n", id);

  return var;
}

JsonObject SysModModel::setValueB(const char * id, bool value, uint8_t rowNr) {
  JsonObject var = findVar(id);
  if (!var.isNull()) {
    // print->printJson("setValueB", var);
    if (rowNr == uint8Max) { //normal situation
      if (var["value"].isNull() || var["value"] != value) {
        USER_PRINTF("setValueB changed %s (%d) %s->%s\n", id, rowNr, var["value"].as<String>().c_str(), value?"true":"false");
        var["value"] = value;
        ui->setChFunAndWs(var, rowNr);
      }
    }
    else {
      //if we deal with multiple rows, value should be an array, if not we create one

      if (var["value"].isNull() || !var["value"].is<JsonArray>()) {
        USER_PRINTF("setValueB var %s (%d) value %s not array, creating\n", id, rowNr, var["value"].as<String>().c_str());
        // print->printJson("setValueB var %s value %s not array, creating", id, var["value"].as<String>().c_str());
        var.createNestedArray("value");
      }

      if (var["value"].is<JsonArray>()) {
        //set the right value in the array (if array did not contain values yet, all values before rownr are set to false)
        if (var["value"][rowNr] != value) {
          var["value"][rowNr] = value;
          ui->setChFunAndWs(var, rowNr);
        }
      }
      else 
        USER_PRINTF("setValueB %s could not create value array\n", id);
    }
  }
  else
    USER_PRINTF("setValueB Var %s not found\n", id);
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
  USER_PRINTF("%s\n", value);

  va_end(args);

  return setValueC(id, value);
}

void SysModModel::setValueLossy(const char * id, const char * format, ...) {

  va_list args;
  va_start(args, format);

  // size_t len = vprintf(format, args);
  char value[128];
  vsnprintf(value, sizeof(value)-1, format, args);

  va_end(args);

  JsonDocument *responseDoc = web->getResponseDoc();
  responseDoc->clear(); //needed for deserializeJson?
  JsonVariant responseVariant = responseDoc->as<JsonVariant>();

  web->addResponse(id, "value", value);

  bool isOk = true;
  for (auto client:SysModWeb::ws->getClients()) {
      if (client->status() != WS_CONNECTED || client->queueIsFull() || client->queueLength()>1) //lossy
        isOk = false;
  }
  if (isOk)
    web->sendDataWs(responseVariant);
  // else
  //   USER_PRINTF("x");
}

JsonVariant SysModModel::getValue(const char * id) {
  JsonObject var = findVar(id);
  if (!var.isNull())
    return var["value"];
  else {
    USER_PRINTF("getValue: Var %s does not exist!!\n", id);
    return JsonVariant();
  }
}

JsonObject SysModModel::findVar(const char * id, JsonArray parent) {
  JsonArray root;
  // print ->print("findVar %s %s\n", id, parent.isNull()?"root":"n");
  if (parent.isNull()) {
    root = model->as<JsonArray>();
    modelParentVar = JsonObject();
  }
  else
    root = parent;

  for (JsonObject var : root) {
    // if (foundVar.isNull()) {
      if (var["id"] == id)
        return var;
      else if (!var["n"].isNull()) {
        JsonObject foundVar = findVar(id, var["n"]);
        if (!foundVar.isNull()) {
          if (modelParentVar.isNull()) modelParentVar = var;  //only recursive lowest assigns parentVar
          // USER_PRINTF("findvar parent of %s is %s\n", id, modelParentVar["id"].as<const char *>());
          return foundVar;
        }
      }
    // }
  }
  return JsonObject();
}

void SysModModel::findVars(const char * property, bool value, FindFun fun, JsonArray parent) {
  JsonArray root;
  // print ->print("findVar %s %s\n", id, parent.isNull()?"root":"n");
  if (parent.isNull())
    root = model->as<JsonArray>();
  else
    root = parent;

  for(JsonObject var : root) {
    if (var[property] == value)
      fun(var);
    if (!var["n"].isNull())
      findVars(property, value, fun, var["n"]);
  }
}