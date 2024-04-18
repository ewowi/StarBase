/*
   @title     StarMod
   @file      SysModPrint.h
   @date      20240411
   @repo      https://github.com/ewowi/StarMod, submit changes to this file as PRs to ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"

#define USER_PRINTF(x...) print->printf(x)
#define prf(x...) print->printf(x)
// #define USER_PRINT_FUNCTION(x...) //print->printf(x)
// #define USER_PRINT_NOT(x...) //print->printf(x)
// #define USER_PRINT_Async(x...) print->printf(x)

class SysModPrint:public SysModule {

public:

  SysModPrint();
  void setup();
  void loop();

  //generic print function
  void printf(const char * format, ...);

  //not used yet
  void println(const __FlashStringHelper * x);

  //print var as id:value + [childVars recursively]
  void printVar(JsonObject var);

  void printJson(const char * text, JsonVariantConst source);

  size_t fFormat(char * buf, size_t size, const char * format, ...);

  void printJDocInfo(const char * text, JsonDocument source);

private:
  bool setupsDone = false;
};

extern SysModPrint *print;