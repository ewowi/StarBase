/*
   @title     StarMod
   @file      SysModModel.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModModel.h"
#include "SysModule.h"
#include "SysModFiles.h"
#include "SysJsonRDWS.h"

BasicJsonDocument<RAM_Allocator> * SysModModel::model = nullptr;
JsonObject SysModModel::modelParentVar;

SysModModel::SysModModel() :SysModule("Model") {
  model = new BasicJsonDocument<RAM_Allocator>(24576);

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

  parentVar = ui->initSysMod(parentVar, name);

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
  setValueV("mSize", "%d / %d B", model->memoryUsage(), model->capacity());
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

void SysModModel::varToValues(JsonObject var, JsonArray row) {

    //add value for each child
    // JsonArray row = rows.createNestedArray();
    for (JsonObject childVar : var["n"].as<JsonArray>()) {
      print->printJson("fxTbl childs", childVar);
      row.add(childVar["value"]);

      if (!childVar["n"].isNull()) {
        varToValues(childVar, row.createNestedArray());
      }
    }

}