/*
   @title     StarMod
   @file      SysModSystem.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include "SysModule.h"

class SysModSystem:public SysModule {

public:
  char version[16] = "";
  char chipInfo[64] = "";

  SysModSystem();
  void setup();
  void loop();
  void loop1s();
  void loop10s();

  //from esp32Tools
  bool sysTools_normal_startup(void);              // FALSE if unusual startup code --> use next function to get more info
  String sysTools_getRestartReason(void);          // long string including restart codes from system, Core#0 and Core#1 (if availeable)
  String sysTools_restart2String(int reasoncode);  // helper for SysModSystem::addRestartReasonsSelect. Returns "(#) ReasonText"
  String sysTools_reset2String(int resetCode);     // helper for SysModSystem::addResetReasonsSelect. Returns "shortResetReasonText (#)"
  int sysTools_get_arduino_maxStackUsage(void);    // to query max used stack of the arduino task. returns "-1" if unknown
  int sysTools_get_webserver_maxStackUsage(void);  // to query max used stack of the webserver task. returns "-1" if unknown

private:
  unsigned long loopCounter = 0;

  void addResetReasonsSelect(JsonArray select);
  void addRestartReasonsSelect(JsonArray select);

  // from esp32Tools: helper fuctions
  int getCoreResetReason(int core);
  String resetCode2Info(int reason);
  esp_reset_reason_t getRestartReason();
  String restartCode2InfoLong(esp_reset_reason_t reason);
  String restartCode2Info(esp_reset_reason_t reason);

  TaskHandle_t loop_taskHandle = NULL;                   // to store the task handle for later calls
  TaskHandle_t tcp_taskHandle = NULL;                   // to store the task handle for later calls

};

extern SysModSystem *sys;