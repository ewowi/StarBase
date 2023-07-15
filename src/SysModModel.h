#pragma once
#include "module.h"

#include "ArduinoJson.h"

class SysModModel:public Module {

public:
  static bool doWriteModel;
  static bool doShowObsolete;

  // StaticJsonDocument<24576> model; //not static as that blows up the stack. Use extern??
  static DynamicJsonDocument *model;

  SysModModel();
  void setup();
  void loop();

  //scan all objects in the model and remove the s element 
  void cleanUpModel(JsonArray objects);

  //reads model.json and load it in model
  bool readObjectFromFile(const char* path, JsonDocument* dest);

  //write model into model.json
  bool writeObjectToFile(const char* path, JsonDocument* dest);

  //sets the value of object with id
  static JsonObject setValue(const char * id, const char * value);

  //setValue int
  static JsonObject setValue(const char * id, int value);

  //setValue bool
  static JsonObject setValue(const char * id, bool value);

  //Set value with argument list
  static JsonObject setValueV(const char * id, const char * format, ...);

  //Set value with argument list and print
  JsonObject setValueP(const char * id, const char * format, ...);

  JsonVariant getValue(const char * id);

  //returns the object defined by id (parent to recursively call findObject)
  static JsonObject findObject(const char * id, JsonArray parent = JsonArray());

};

static SysModModel *mdl;