#pragma once
#include "Module.h"

class SysModPrint:public Module {

public:

  SysModPrint();
  void setup();
  void loop();

  //generic print function (based on printf)
  size_t print(const char * format, ...);

  size_t println(const __FlashStringHelper * x);

  void printObject(JsonObject object);

  size_t printJson(const char * text, JsonVariantConst source);

  const char * fFormat(const char * format, ...);
};

static SysModPrint *print;