/*
   @title     StarMod
   @file      SysModModel.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"

#include "ArduinoJson.h"

typedef std::function<void(JsonObject)> FindFun;
typedef std::function<void(JsonObject, size_t)> ChangeFun;

struct Coord3D {
  uint16_t x;
  uint16_t y;
  uint16_t z;
  bool operator!=(Coord3D rhs) {
    // USER_PRINTF("Coord3D compare%d %d %d %d %d %d\n", x, y, z, par.x, par.y, par.z);
    return x != rhs.x || y != rhs.y || z != rhs.z;
  }
  bool operator==(Coord3D rhs) {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }
};

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

  JsonVariant getValue(const char * id, uint8_t rowNr = uint8Max);

  // //tbd
  // template <typename Type>
  // Type getValue2(const char * id) {
  //   return getValue(id).as<Type>();
  // };

  //returns the var defined by id (parent to recursively call findVar)
  static JsonObject findVar(const char * id, JsonArray parent = JsonArray()); //static for processJson
  void findVars(const char * id, bool value, FindFun fun, JsonArray parent = JsonArray());

  //recursively add values in  a variant
  void varToValues(JsonObject var, JsonArray values);
  JsonVariant varToValue(JsonObject var, u_int8_t rowNr);

private:
  bool doWriteModel = false;
  bool doShowObsolete = false;
  bool cleanUpModelDone = false;

};

static SysModModel *mdl;