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
  static std::vector<VarFun> varFunctions; //static because of static functions callChFunAndWs, processJson...

  SysModUI();

  //serve index.htm
  void setup();

  void loop();
  void loop1s();

  JsonObject initAppMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<const char *>(parent, id, "appmod", value, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }
  JsonObject initSysMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<const char *>(parent, id, "sysmod", value, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }
  JsonObject initUserMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<const char *>(parent, id, "usermod", value, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<const char *>(parent, id, "table", value, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, uint8_t rowNr = UINT8_MAX, uint16_t max = 32, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<const char *>(parent, id, "text", value, rowNr, 0, max, readOnly, varFun, nrOfRows);
  }

  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, uint8_t max = 32, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<const char *>(parent, id, "password", value, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<int>(parent, id, "number", value, UINT8_MAX, min, max, readOnly, varFun, nrOfRows);
  }

  JsonObject initProgress(JsonObject parent, const char * id, int value = UINT16_MAX, uint8_t rowNr = UINT8_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<int>(parent, id, "progress", value, rowNr, min, max, readOnly, varFun, nrOfRows);
  }

  JsonObject initCoord3D(JsonObject parent, const char * id, Coord3D value = {UINT16_MAX, UINT16_MAX, UINT16_MAX}, uint8_t rowNr = UINT8_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<Coord3D>(parent, id, "coord3D", value, rowNr, min, max, readOnly, varFun, nrOfRows);
  }

  //init a range slider, range between 0 and 255!
  JsonObject initSlider(JsonObject parent, const char * id, int value = UINT16_MAX, uint8_t rowNr = UINT8_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<int>(parent, id, "range", value, rowNr, min, max, readOnly, varFun, nrOfRows);
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<int>(parent, id, "canvas", value, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }

  //supports 3 state value: if UINT16_MAX it is indeterminated
  JsonObject initCheckBox(JsonObject parent, const char * id, int value = UINT16_MAX, uint8_t rowNr = UINT8_MAX, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<int>(parent, id, "checkbox", value, rowNr, 0, 0, readOnly, varFun, nrOfRows);
  }

  //a button never gets a value
  JsonObject initButton(JsonObject parent, const char * id, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<bool>(parent, id, "button", false, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }

  JsonObject initSelect(JsonObject parent, const char * id, int value = UINT16_MAX, uint8_t rowNr = UINT8_MAX, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<int>(parent, id, "select", value, rowNr, 0, 0, readOnly, varFun, nrOfRows);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<const char *>(parent, id, "textarea", value, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }

  JsonObject initURL(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    return initVarAndUpdate<const char *>(parent, id, "url", value, UINT8_MAX, 0, 0, readOnly, varFun, nrOfRows);
  }

  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type value, uint8_t rowNr = UINT8_MAX, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr, uint8_t nrOfRows = 0) {
    JsonObject var = initVar(parent, id, type, readOnly, varFun);
    if (min) var["min"] = min;
    if (max && max != UINT16_MAX) var["max"] = max;

    bool valueNeedsUpdate = false;
    if (strcmp(type, "button") != 0) { //button never gets a value
      if (var["value"].isNull()) {
        valueNeedsUpdate = true;
        // print->printJson("initVarAndUpdate varFun value is null", var);
      } else if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = var["value"].as<JsonArray>();
        if (rowNr != UINT8_MAX) {
          if (rowNr >= valueArray.size())
            valueNeedsUpdate = true;
          else if (valueArray[rowNr].isNull())
            valueNeedsUpdate = true;
        }
        else if (nrOfRows && valueArray.size() != nrOfRows) {
          print->printJson("initVarAndUpdate varFun value array wrong size", var);
          valueNeedsUpdate = true;
        }
      }
    }

    if (valueNeedsUpdate) {
      bool valueFunExists = false;
      if (varFun) {
        USER_PRINTF("initVarAndUpdate valueFun %s nrOfRows:%d\n", id, nrOfRows);
        for (int rowNr=0;rowNr<nrOfRows;rowNr++) {
          valueFunExists = varFun(var, rowNr, f_ValueFun);
          if (!valueFunExists)
            break;
        }
      }
      if (!valueFunExists)
        mdl->setValue(var, value, rowNr); //does changefun if needed
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
        else
          changeFunExists = varFun(var, 0, f_ChangeFun); //if no rowNr use rowNr 0

        if (changeFunExists)
          USER_PRINTF("initVarAndUpdate chFun V2 !!! init %s v:%s\n", var["id"].as<const char *>(), var["value"].as<String>().c_str());
      }
    }

    return var;
  }

  JsonObject initVar(JsonObject parent, const char * id, const char * type, bool readOnly = true, VarFun varFun = nullptr);

  uint8_t callVarFun(JsonObject var, uint8_t rowNr, uint8_t funType) {
    uint8_t result = false;
    if (!var["fun"].isNull()) {//isNull needed here!
      size_t funNr = var["fun"];
      if (funNr < varFunctions.size()) {
        result = varFunctions[funNr](var, rowNr, funType);
        if (result) { //send rowNr = 0 if no rowNr
          //only print vars with a value
          if (!var["value"].isNull()) {
            USER_PRINTF("%sFun %s", funType==f_ValueFun?"val":funType==f_UIFun?"ui":funType==f_ChangeFun?"ch":funType==f_AddRow?"add":funType==f_DelRow?"del":"other", var["id"].as<const char *>());
            if (rowNr!=UINT8_MAX)
              USER_PRINTF("[%d]", rowNr);
            USER_PRINTF(" <- %s\n", var["value"].as<String>().c_str());
          }
        }
      }
      else    
        USER_PRINTF("dev callChFunAndWs function nr %s outside bounds %d >= %d\n", var["id"].as<const char *>(), funNr, varFunctions.size());
    }
    return result;
  }

  //interpret json and run commands or set values like deserializeJson / deserializeState / deserializeConfig
  void processJson(JsonVariant json); //static for jsonHandler, must be Variant, not object for jsonhandler

  //called to rebuild selects and tables (tbd: also label and comments is done again, that is not needed)
  void processUiFun(const char * id);

private:
  static int varCounter; //not static crashes ??? (not called async...?)

  static std::vector<VarLoop> loopFunctions; //non static crashing ...

};

static SysModUI *ui;