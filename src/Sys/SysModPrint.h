/*
   @title     StarMod
   @file      SysModPrint.h
   @date      20230729
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
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

  //experimenting with return of char, if possible at all - wip... use String?
  char * fFormat(const char * format, ...);

  void printJDocInfo(const char * text, DynamicJsonDocument source);

private:
  bool setupsDone = false;
};

static SysModPrint *print;