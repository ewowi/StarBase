/*
   @title     StarMod
   @file      esp32Tools.h
   @date      20240121
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

// This is a collection of ESP32 specific low-level functions.

bool sysTools_normal_startup(void);              // FALSE if unusual startup code --> use next function to get more info
String sysTools_getRestartReason(void);          // long string including restart codes from system, Core#0 and Core#1 (if availeable)

String sysTools_restart2String(int reasoncode);  // helper for SysModSystem::addRestartReasonsSelect. Returns "(#) ReasonText"
String sysTools_reset2String(int resetCode);     // helper for SysModSystem::addResetReasonsSelect. Returns "shortResetReasonText (#)"

int sysTools_get_arduino_maxStackUsage(void);    // to query max used stack of the arduino task. returns "-1" if unknown
int sysTools_get_webserver_maxStackUsage(void);  // to query max used stack of the webserver task. returns "-1" if unknown
