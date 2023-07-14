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

  void cleanUpModel(JsonArray objects);

  bool readObjectFromFile(const char* path, JsonDocument* dest);

  bool writeObjectToFile(const char* path, JsonDocument* dest);

};

static SysModModel *mdl;