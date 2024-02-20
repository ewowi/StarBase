/*
   @title     StarMod
   @file      SysModUI.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "SysModPrint.h"
#include "SysModModel.h"
#include "SysModules.h" //isConnected

enum FunIDs
{
  f_ValueFun,
  f_UIFun,
  f_ChangeFun,
  f_LoopFun,
  f_AddRow,
  f_DelRow,
  f_count
};

// https://stackoverflow.com/questions/59111610/how-do-you-declare-a-lambda-function-using-typedef-and-then-use-it-by-passing-to
typedef std::function<uint8_t(JsonObject, uint8_t, uint8_t)> VarFun;
// typedef void(*LoopFun)(JsonObject, uint8_t*); //std::function is crashing...
// typedef std::function<void(JsonObject, uint8_t)> LoopFun;

struct VarLoop {
  JsonObject var;
  VarFun loopFun;
  unsigned long lastMillis = 0;
  unsigned long counter = 0;
};

static uint8_t linearToLogarithm(JsonObject var, uint8_t value) {
  if (value == 0) return 0;

  float minp = var["min"].isNull()?var["min"]:0;
  float maxp = var["max"].isNull()?var["max"]:255;

  // The result should be between 100 an 10000000
  float minv = minp?log(minp):0;
  float maxv = log(maxp);

  // calculate adjustment factor
  float scale = (maxv-minv) / (maxp-minp);

  return round(exp(minv + scale*((float)value-minp)));
}

class SysModUI:public SysModule {

public:
  static bool stageVarChanged;// = false; //tbd: move mechanism to UserModInstances as there it will be used
  static std::vector<VarFun> varFunctions; //static because of static functions callChangeFun, processJson...

  static uint8_t parentRowNr;

  SysModUI();

  //serve index.htm
  void setup();

  void loop();
  void loop1s();

  JsonObject initAppMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "appmod", value, 0, 0, readOnly, varFun);
  }
  JsonObject initSysMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "sysmod", value, 0, 0, readOnly, varFun);
  }
  JsonObject initUserMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "usermod", value, 0, 0, readOnly, varFun);
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "table", value, 0, 0, readOnly, varFun);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, uint16_t max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "text", value, 0, max, readOnly, varFun);
  }

  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, uint8_t max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "password", value, 0, 0, readOnly, varFun);
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "number", value, min, max, readOnly, varFun);
  }

  JsonObject initProgress(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "progress", value, min, max, readOnly, varFun);
  }

  JsonObject initCoord3D(JsonObject parent, const char * id, Coord3D value = {UINT16_MAX, UINT16_MAX, UINT16_MAX}, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varFun);
  }

  //init a range slider, range between 0 and 255!
  JsonObject initSlider(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "range", value, min, max, readOnly, varFun);
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "canvas", value, 0, 0, readOnly, varFun);
  }

  //supports 3 state value: if UINT16_MAX it is indeterminated
  JsonObject initCheckBox(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "checkbox", value, 0, 0, readOnly, varFun);
  }

  //a button never gets a value
  JsonObject initButton(JsonObject parent, const char * id, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<bool>(parent, id, "button", false, 0, 0, readOnly, varFun);
  }

  JsonObject initSelect(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "select", value, 0, 0, readOnly, varFun);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "textarea", value, 0, 0, readOnly, varFun);
  }

  JsonObject initURL(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "url", value, 0, 0, readOnly, varFun);
  }

  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type value, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr) {
    JsonObject var = initVar(parent, id, type, readOnly, varFun);
    if (min) var["min"] = min;
    if (max && max != UINT16_MAX) var["max"] = max;

    if (parentRowNr != UINT8_MAX) {
      USER_PRINTF("parentRowNr %s: %d for %s\n", mdl->varID(parent), parentRowNr, mdl->varID(var));
    }

    bool valueNeedsUpdate = false;
    if (strcmp(type, "button") != 0) { //button never gets a value
      if (var["value"].isNull()) {
        valueNeedsUpdate = true;
        // print->printJson("initVarAndUpdate varFun value is null", var);
      } else if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = var["value"].as<JsonArray>();
        if (parentRowNr != UINT8_MAX) { // if var in table
          if (parentRowNr >= valueArray.size())
            valueNeedsUpdate = true;
          else if (valueArray[parentRowNr].isNull())
            valueNeedsUpdate = true;
        }
      }
    }

    if (valueNeedsUpdate) {
      bool valueFunExists = false;
      if (varFun) {
        valueFunExists = varFun(var, parentRowNr, f_ValueFun);
      }
      if (!valueFunExists) { //setValue provided (if not null)
        if (mdl->varRO(var) && !mdls->isConnected) {
          mdl->setValueNoROCheck(var, value, parentRowNr); //does changefun if needed, if var in table, update the table row
        } else
          mdl->setValue(var, value, parentRowNr); //does changefun if needed, if var in table, update the table row
      }
    }
    else { //do changeFun on existing value
      //no call of chFun for buttons otherwise all buttons will be fired which is highly undesirable
      if (strcmp(type,"button") != 0 && varFun ) { //!isPointer because 0 is also a value then && (!isPointer || value)
        bool changeFunExists = false;
        if (var["value"].is<JsonArray>()) {
          int rowNr = 0;
          for (JsonVariant val:var["value"].as<JsonArray>()) {
            changeFunExists |= varFun(var, rowNr++, f_ChangeFun);
          }
        }
        else {
          changeFunExists = varFun(var, UINT8_MAX, f_ChangeFun); //if no rowNr use rowNr 0
        }

        if (changeFunExists)
          USER_PRINTF("initVarAndUpdate chFun init %s[x] <- %s\n", mdl->varID(var), var["value"].as<String>().c_str());
      }
    }

    if (parentRowNr != UINT8_MAX)
      print->printJson("gravity var", web->getResponseObject());

    return var;
  }

  JsonObject initVar(JsonObject parent, const char * id, const char * type, bool readOnly = true, VarFun varFun = nullptr);

  uint8_t callVarFun(const char * varID, uint8_t rowNr = UINT8_MAX, uint8_t funType = f_ChangeFun) {
    JsonObject var = mdl->findVar(varID);
    return callVarFun(var, rowNr, funType);
  }

  uint8_t callVarFun(JsonObject var, uint8_t rowNr = UINT8_MAX, uint8_t funType = f_ValueFun) {
    uint8_t result = false;

    if (!var["fun"].isNull()) {//isNull needed here!
      size_t funNr = var["fun"];
      if (funNr < varFunctions.size()) {
        result = varFunctions[funNr](var, rowNr, funType);
        // if (result) { //send rowNr = 0 if no rowNr
        //   //only print vars with a value
        //   if (!var["value"].isNull() && funType != f_ValueFun) {
        //     USER_PRINTF("%sFun %s", funType==f_ValueFun?"val":funType==f_UIFun?"ui":funType==f_ChangeFun?"ch":funType==f_AddRow?"add":funType==f_DelRow?"del":"other", mdl->varID(var));
        //     if (rowNr != UINT8_MAX)
        //       USER_PRINTF("[%d] = %s\n", rowNr, var["value"][rowNr].as<String>().c_str());
        //     else
        //       USER_PRINTF(" = %s\n", var["value"].as<String>().c_str());
        //   }
        // }
      }
      else    
        USER_PRINTF("dev callVarFun function nr %s outside bounds %d >= %d\n", mdl->varID(var), funNr, varFunctions.size());
    }

    //for ro variables, call valueFun to add also the value in responseDoc (as it is not stored in the model)
    if (funType == f_UIFun && mdl->varRO(var)) {
      callVarFun(var, rowNr, f_ValueFun);
    }

    return result;
  }

  //interpret json and run commands or set values like deserializeJson / deserializeState / deserializeConfig
  void processJson(JsonVariant json); //static for jsonHandler, must be Variant, not object for jsonhandler

  //called to rebuild selects and tables (tbd: also label and comments is done again, that is not needed)
  // void processUiFun(const char * id);

  void setLabel(JsonObject var, const char * text) {
    web->addResponse(var["id"], "label", text);
  }
  void setComment(JsonObject var, const char * text) {
    web->addResponse(var["id"], "comment", text);
  }
  JsonArray setOptions(JsonObject var) {
    return web->addResponseA(var["id"], "options");
  }

private:
  static int varCounter; //not static crashes ??? (not called async...?)

  static std::vector<VarLoop> loopFunctions; //non static crashing ...

};

static SysModUI *ui;