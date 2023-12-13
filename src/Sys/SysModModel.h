/*
   @title     StarMod
   @file      SysModModel.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#pragma once
#include "SysModule.h"

#include "ArduinoJson.h"

typedef std::function<void(JsonObject)> FindFun;
typedef std::function<void(JsonObject, size_t)> ChangeFun;

class SysModModel:public SysModule {

public:

  // StaticJsonDocument<24576> model; //not static as that blows up the stack. Use extern??
  static DynamicJsonDocument *model; //needs to be static as loopTask and asyncTask is using it...

  static JsonObject modelParentVar;

  SysModModel();
  void setup();
  void loop();
  void loop1s();

  //scan all vars in the model and remove vars where var["o"] is negative or positive, if ro then remove ro values
  void cleanUpModel(JsonArray vars, bool oPos = true, bool ro = false);

  //sets the value of var with id
  static JsonObject setValueC(const char * id, const char * value, uint8_t rowNr = uint8Max);

  //setValue int
  static JsonObject setValueI(const char * id, int value, uint8_t rowNr = uint8Max);

  //setValue bool
  static JsonObject setValueB(const char * id, bool value, uint8_t rowNr = uint8Max);

  //Set value with argument list
  static JsonObject setValueV(const char * id, const char * format, ...); //static to use in *Fun

  //Set value with argument list and print
  JsonObject setValueP(const char * id, const char * format, ...);

  //Send value directly to ws (tbd: no model function but web?)
  void setValueLossy(const char * id, const char * format, ...);

  //tbd
  // template <typename Type>
  // static JsonObject setValue(const char * id, Type value) {
  //   if (std::is_same<Type, const char *>::value)
  //     return setValueC(id, (const char *)value);
  //   if (std::is_same<Type, int>::value)
  //     return setValueI(id, value);
  //   if (std::is_same<Type, bool>::value)
  //     return setValueB(id, value);
  //   return JsonObject();
  // };

  JsonVariant getValue(const char * id);

  // //tbd
  // template <typename Type>
  // Type getValue2(const char * id) {
  //   return getValue(id).as<Type>();
  // };

  //returns the var defined by id (parent to recursively call findVar)
  static JsonObject findVar(const char * id, JsonArray parent = JsonArray()); //static for processJson
  void findVars(const char * id, bool value, FindFun fun, JsonArray parent = JsonArray());

private:
  bool doWriteModel = false;
  bool doShowObsolete = false;
  bool cleanUpModelDone = false;

};

static SysModModel *mdl;