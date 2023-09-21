/*
   @title     StarMod
   @file      SysModPrint.h
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

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

  void printVar(JsonObject var);

  size_t printJson(const char * text, JsonVariantConst source);

  size_t fFormat(char * buf, size_t size, const char * format, ...);

  void printJDocInfo(const char * text, DynamicJsonDocument source);

private:
  bool setupsDone = false;
};

static SysModPrint *print;