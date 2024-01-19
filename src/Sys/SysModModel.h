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
#include "SysModPrint.h"
#include "SysModUI.h"

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

//https://arduinojson.org/news/2021/05/04/version-6-18-0/
namespace ArduinoJson {
  template <>
  struct Converter<Coord3D> {
    static bool toJson(const Coord3D& src, JsonVariant dst) {
      dst["x"] = src.x;
      dst["y"] = src.y;
      dst["z"] = src.z;
      return true;
    }

    static Coord3D fromJson(JsonVariantConst src) {
      return Coord3D{src["x"], src["y"], src["z"]};
    }

    static bool checkJson(JsonVariantConst src) {
      return src["x"].is<int>() && src["y"].is<int>() && src["z"].is<int>();
    }
  };
}

// https://arduinojson.org/v6/api/basicjsondocument/
struct RAM_Allocator {
  void* allocate(size_t size) {
    if (psramFound()) return ps_malloc(size); // use PSRAM if it exists
    else              return malloc(size);    // fallback
    // return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }
  void deallocate(void* pointer) {
    free(pointer);
    // heap_caps_free(pointer);
  }
  void* reallocate(void* ptr, size_t new_size) {
    if (psramFound()) return ps_realloc(ptr, new_size); // use PSRAM if it exists
    else              return realloc(ptr, new_size);    // fallback
    // return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};



class SysModModel:public SysModule {

public:

  // StaticJsonDocument<24576> model; //not static as that blows up the stack. Use extern??
  // static BasicJsonDocument<DefaultAllocator> *model; //needs to be static as loopTask and asyncTask is using it...
  static BasicJsonDocument<RAM_Allocator> *model; //needs to be static as loopTask and asyncTask is using it...

  static JsonObject modelParentVar;

  SysModModel();
  void setup();
  void loop();
  void loop1s();

  //scan all vars in the model and remove vars where var["o"] is negative or positive, if ro then remove ro values
  void cleanUpModel(JsonArray vars, bool oPos = true, bool ro = false);

  //sets the value of var with id
  template <typename Type>
  JsonObject setValue(const char * id, Type value, uint8_t rowNr = uint8Max) {
    JsonObject var = findVar(id);
    if (!var.isNull()) {
      // print->printJson("setValueB", var);
      if (rowNr == uint8Max) { //normal situation
        bool same;
        // if (std::is_same<Type, const char *>::value)
        //   same = var["value"] != value;
        // else
          same = var["value"].as<Type>() != value;
        if (var["value"].isNull() || same) {
          USER_PRINTF("setValue changed %s (%d) %s->..\n", id, rowNr, var["value"].as<String>().c_str());//, value?"true":"false");
          var["value"] = value;
          ui->setChFunAndWs(var, rowNr);
        }
      }
      else {
        //if we deal with multiple rows, value should be an array, if not we create one

        if (var["value"].isNull() || !var["value"].is<JsonArray>()) {
          USER_PRINTF("setValue var %s (%d) value %s not array, creating\n", id, rowNr, var["value"].as<String>().c_str());
          // print->printJson("setValueB var %s value %s not array, creating", id, var["value"].as<String>().c_str());
          var.createNestedArray("value");
        }

        if (var["value"].is<JsonArray>()) {
          //set the right value in the array (if array did not contain values yet, all values before rownr are set to false)
          bool same;
          // if (std::is_same<Type, const char *>::value)
          //   same = var["value"][rowNr] != value;
          // else
            same = var["value"][rowNr].as<Type>() != value;
          if (same) {
            var["value"][rowNr] = value;
            ui->setChFunAndWs(var, rowNr);
          }
        }
        else 
          USER_PRINTF("setValue %s could not create value array\n", id);
      }
    }
    else
      USER_PRINTF("setValue Var %s not found\n", id);
    return var;
  }

  //Set value with argument list
  JsonObject setValueV(const char * id, const char * format, ...); //static to use in *Fun

  //Set value with argument list and print
  JsonObject setValueP(const char * id, const char * format, ...);

  //Send value directly to ws (tbd: no model function but web?)
  void setValueLossy(const char * id, const char * format, ...);

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