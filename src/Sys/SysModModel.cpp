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
#include "SysModUI.h"

JsonDocument * SysModModel::model = nullptr;
JsonObject SysModModel::modelParentVar;

SysModModel::SysModModel() :SysModule("Model") {
  model = new JsonDocument(&allocator);

  JsonArray root = model->to<JsonArray>(); //create

  USER_PRINTF("Reading model from /model.json... (deserializeConfigFromFS)\n");
  if (files->readObjectFromFile("/model.json", model)) {//not part of success...
    // print->printJson("Read model", *model);
    // web->sendDataWs(*model);
  } else {
    root = model->to<JsonArray>(); //re create the model as it is corrupted by readFromFile
  }
}

void SysModModel::setup() {
  SysModule::setup();

  parentVar = ui->initSysMod(parentVar, name);
  if (parentVar["o"] > -1000) parentVar["o"] = -4000; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

  ui->initButton(parentVar, "saveModel", false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setComment(var, "Write to model.json");
      return true;
    case f_ChangeFun:
      doWriteModel = true;
      return true;
    default: return false;
  }});

  ui->initCheckBox(parentVar, "showObsolete", doShowObsolete, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setComment(var, "Show in UI (refresh)");
      return true;
    case f_ChangeFun:
      doShowObsolete = var["value"];
      return true;
    default: return false;
  }});

  ui->initButton(parentVar, "deleteObsolete", false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Delete obsolete variables");
      ui->setComment(var, "WIP");
      return true;
    case f_ChangeFun:
      model->to<JsonArray>(); //create
      if (files->readObjectFromFile("/model.json", model)) {//not part of success...
      }
      return true;
    default: return false;
  }});
}

  void SysModModel::loop() {
  // SysModule::loop();

  if (!cleanUpModelDone) { //do after all setups
    cleanUpModelDone = true;
    cleanUpModel();
  }

  if (doWriteModel) {
    USER_PRINTF("Writing model to /model.json... (serializeConfig)\n");

    // files->writeObjectToFile("/model.json", model);

    cleanUpModel(JsonObject(), false, true);//remove if var["o"] is negative (not cleanedUp) and remove ro values

    JsonRDWS jrdws("/model.json", "w"); //open fileName for deserialize
    jrdws.addExclusion("fun");
    jrdws.writeJsonDocToFile(model);

    // print->printJson("Write model", *model); //this shows the model before exclusion

    doWriteModel = false;
  }
}

void SysModModel::cleanUpModel(JsonObject parent, bool oPos, bool ro) {

  JsonArray vars;
  if (parent.isNull()) //no parent
    vars = model->as<JsonArray>();
  else
    vars = varN(parent);

  for (JsonArray::iterator varV=vars.begin(); varV!=vars.end(); ++varV) {
  // for (JsonVariant varV : vars) {
    if (varV->is<JsonObject>()) {
      JsonObject var = *varV;

      //no cleanup of o in case of ro value removal
      if (!ro) {
        if (oPos) {
          if (var["o"].isNull() || varOrder(var) >= 0) { //not set negative in initVar
            if (!doShowObsolete) {
              USER_PRINTF("cleanUpModel remove var %s (""o"">=0)\n", varID(var));          
              vars.remove(varV); //remove the obsolete var (no o or )
            }
          }
          else {
            varOrder(var, -varOrder(var)); //make it possitive
          }
        } else { //!oPos
          if (var["o"].isNull() || varOrder(var) < 0) { 
            USER_PRINTF("cleanUpModel remove var %s (""o""<0)\n", varID(var));          
            vars.remove(varV); //remove the obsolete var (no o or o is negative - not cleanedUp)
          }
        }
      }

      //remove ro values (ro vars cannot be deleted as SM uses these vars)
      // remove if var is ro or table of var is ro (ro table can have non ro vars e.g. instance table)
      if (ro && ((parent["type"] == "table" && varRO(parent)) || varRO(var))) {// && !var["value"].isNull())
      // if (ro && varRO(var)) {// && !var["value"].isNull())
        USER_PRINTF("remove ro value %s\n", varID(var));          
        var.remove("value");
      }

      //recursive call
      if (!varN(var).isNull())
        cleanUpModel(var, oPos, ro);
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
          // USER_PRINTF("findvar parent of %s is %s\n", id, varID(modelParentVar));
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

  for (JsonObject var : root) {
    if (var[property] == value)
      fun(var);
    if (!var["n"].isNull())
      findVars(property, value, fun, var["n"]);
  }
}

void SysModModel::varToValues(JsonObject var, JsonArray row) {

    //add value for each child
    // JsonArray row = rows.add<JsonArray>();
    for (JsonObject childVar : var["n"].as<JsonArray>()) {
      print->printJson("fxTbl childs", childVar);
      row.add(childVar["value"]);

      if (!childVar["n"].isNull()) {
        varToValues(childVar, row.add<JsonArray>());
      }
    }
}

void SysModModel::callChangeFun(JsonObject var, uint8_t rowNr) {

  //done here as ui cannot be used in SysModModel.h
  if (var["stage"])
    ui->stageVarChanged = true;

  ui->callVarFun(var, rowNr, f_ChangeFun);

  // web->sendResponseObject();
}  