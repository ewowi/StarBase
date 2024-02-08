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
#include <vector>
#include "ArduinoJson.h"
#include "SysModule.h"
#include "SysModPrint.h"
#include "SysModModel.h"

// https://stackoverflow.com/questions/59111610/how-do-you-declare-a-lambda-function-using-typedef-and-then-use-it-by-passing-to
typedef std::function<void(JsonObject)> UFun;
typedef std::function<void(JsonObject, uint8_t)> CFun;
// typedef void(*LoopFun)(JsonObject, uint8_t*); //std::function is crashing...
// typedef std::function<void(JsonObject, uint8_t)> LoopFun;

struct VarLoop {
  JsonObject var;
  CFun loopFun;
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
  static std::vector<UFun> uFunctions; //static because of static functions callChFunAndWs, processJson...
  static std::vector<CFun> cFunctions; //static because of static functions callChFunAndWs, processJson...

  SysModUI();

  //serve index.htm
  void setup();

  void loop();
  void loop1s();

  JsonObject initAppMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "appmod", value, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }
  JsonObject initSysMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "sysmod", value, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }
  JsonObject initUserMod(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "usermod", value, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "table", value, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, uint8_t rowNr = UINT8_MAX, uint16_t max = 32, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "text", value, rowNr, 0, max, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, uint8_t max = 32, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "password", value, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "number", value, UINT8_MAX, min, max, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initProgress(JsonObject parent, const char * id, int value = UINT16_MAX, uint8_t rowNr = UINT8_MAX, int min = 0, int max = 255, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "progress", value, rowNr, min, max, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initCoord3D(JsonObject parent, const char * id, Coord3D value = {UINT16_MAX, UINT16_MAX, UINT16_MAX}, uint8_t rowNr = UINT8_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<Coord3D>(parent, id, "coord3D", value, rowNr, min, max, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  //init a range slider, range between 0 and 255!
  JsonObject initSlider(JsonObject parent, const char * id, int value = UINT16_MAX, uint8_t rowNr = UINT8_MAX, int min = 0, int max = 255, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "range", value, rowNr, min, max, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "canvas", value, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  //supports 3 state value: if UINT16_MAX it is indeterminated
  JsonObject initCheckBox(JsonObject parent, const char * id, int value = UINT16_MAX, uint8_t rowNr = UINT8_MAX, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "checkbox", value, rowNr, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  //a button never gets a value
  JsonObject initButton(JsonObject parent, const char * id, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<bool>(parent, id, "button", false, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initSelect(JsonObject parent, const char * id, int value = UINT16_MAX, uint8_t rowNr = UINT8_MAX, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "select", value, rowNr, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "textarea", value, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  JsonObject initURL(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "url", value, UINT8_MAX, 0, 0, readOnly, uiFun, chFun, loopFun, count, valueFun);
  }

  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type value, uint8_t rowNr = UINT8_MAX, int min = 0, int max = 255, bool readOnly = true, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr, uint8_t count = 0, CFun valueFun = nullptr) {
    JsonObject var = initVar(parent, id, type, readOnly, uiFun, chFun, loopFun);
    if (min) var["min"] = min;
    if (max && max != UINT16_MAX) var["max"] = max;

    bool valueNeedsUpdate = false;
    if (strcmp(type, "button") != 0) { //button never gets a value
      if (var["value"].isNull()) {
        valueNeedsUpdate = true;
        // print->printJson("initVarAndUpdate uiFun value is null", var);
      } else if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = var["value"].as<JsonArray>();
        if (rowNr != UINT8_MAX) {
          if (rowNr >= valueArray.size())
            valueNeedsUpdate = true;
          else if (valueArray[rowNr].isNull())
            valueNeedsUpdate = true;
        }
        else if (count && valueArray.size() != count) {
          print->printJson("initVarAndUpdate uiFun value array wrong size", var);
          valueNeedsUpdate = true;
        }
      }
    }

    if (valueNeedsUpdate) {
      if (!valueFun) {
        mdl->setValue(var, value, rowNr); //does changefun if needed
      }
      else {
        USER_PRINTF("initVarAndUpdate valueFun %s count:%d\n", id, count);
        for (int rowNr=0;rowNr<count;rowNr++)
          valueFun(var, rowNr); //valueFun need to do its own changefun
      }
    }
    else { //do changeFun on existing value
      //no call of chFun for buttons otherwise all buttons will be fired which is highly undesirable
      if (strcmp(type,"button") != 0 && chFun ) { //!isPointer because 0 is also a value then && (!isPointer || value)
        USER_PRINTF("initVarAndUpdate chFun init %s v:%s\n", var["id"].as<const char *>(), var["value"].as<String>().c_str());
        if (var["value"].is<JsonArray>()) {
          int rowNr = 0;
          for (JsonVariant val:var["value"].as<JsonArray>()) {
            chFun(var, rowNr++);
          }
        }
        else
          chFun(var, 0); //if no rowNr use rowNr 0
      }
    }

    return var;
  }

  JsonObject initVar(JsonObject parent, const char * id, const char * type, bool readOnly = true, UFun uiFun = nullptr, CFun chFun = nullptr, CFun loopFun = nullptr);

  //interpret json and run commands or set values like deserializeJson / deserializeState / deserializeConfig
  void processJson(JsonVariant json); //static for jsonHandler, must be Variant, not object for jsonhandler

  //called to rebuild selects and tables (tbd: also label and comments is done again, that is not needed)
  void processUiFun(const char * id);

private:
  static int varCounter; //not static crashes ??? (not called async...?)

  static std::vector<VarLoop> loopFunctions; //non static crashing ...

};

static SysModUI *ui;