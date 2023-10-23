/*
   @title     StarMod
   @file      SysModUI.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include <vector>
#include "ArduinoJson.h"
#include "Module.h"

typedef void(*UCFun)(JsonObject);
typedef void(*LoopFun)(JsonObject, uint8_t*);

struct VarLoop {
  JsonObject var;
  LoopFun loopFun;
  size_t bufSize = 100;
  uint16_t interval = 160; //160ms default
  unsigned long lastMillis = 0;
  unsigned long counter = 0;
  unsigned long prevCounter = 0;
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

class SysModUI:public Module {

public:
  static bool valChangedForInstancesTemp;

  SysModUI();

  //serve index.htm
  void setup();

  void loop();

  JsonObject initModule(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "module", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "table", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, uint16_t max = 32, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "text", value, 0, max, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, uint8_t max = 32, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "password", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value, int min = 0, int max = (uint16_t)-1, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "number", value, min, max, readOnly, uiFun, chFun, loopFun);
  }

  //init a range slider, range between 0 and 255!
  JsonObject initSlider(JsonObject parent, const char * id, int value, int min = 0, int max = 255, int log = 0, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "range", value, min, max, readOnly, uiFun, chFun, loopFun,  { log });
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "canvas", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initCheckBox(JsonObject parent, const char * id, bool value = false, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<bool>(parent, id, "checkbox", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initButton(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "button", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initSelect(JsonObject parent, const char * id, uint8_t value, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<uint8_t>(parent, id, "select", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "textarea", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initURL(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "url", value, 0, 0, readOnly, uiFun, chFun, loopFun);
  }

  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type value, int min, int max, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr, std::initializer_list<int> custom = {}) {
    JsonObject var = initVar(parent, id, type, readOnly, uiFun, chFun, loopFun);
    bool isPointer = std::is_pointer<Type>::value;
    //set a default if not a value yet
    if (var["value"].isNull() && (!isPointer || value)) {
      bool isChar = std::is_same<Type, const char *>::value;
      if (isChar)
        var["value"] = (char *)value; //if char make a copy !!
      else
        var["value"] = value; //if value is a pointer, it needs to have a value
    }
    
    if (min) var["min"] = min;
    if (max) var["max"] = max;

    //custom vars
    uint8_t i = 0;
    for (int c: custom) {
      switch (i) {
        case 0: if (c) var["log"] = true; break; //0 is log WIP!!
      }
      i++;
    };

    //no call of fun for buttons otherwise all buttons will be fired including restart delete model.json and all that jazz!!! 
    if (strcmp(type,"button")!=0 && chFun && (!isPointer || value)) chFun(var); //!isPointer because 0 is also a value then
    return var;
  }

  JsonObject initVar(JsonObject parent, const char * id, const char * type, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr);

  //run the change function and send response to all? websocket clients
  static void setChFunAndWs(JsonObject var, const char * value = nullptr);

  //interpret json and run commands or set values
  static const char * processJson(JsonVariant &json); //static for setupJsonHandlers

  //called to rebuild selects and tables (tbd: also label and comments is done again, that is not needed)
  void processUiFun(const char * id);

private:
  static bool varLoopsChanged;

  static int varCounter; //not static crashes ??? (not called async...?)

  static std::vector<UCFun> ucFunctions;
  static std::vector<VarLoop> loopFunctions;

};

static SysModUI *ui;