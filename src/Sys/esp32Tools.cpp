/*
   @title     StarMod
   @file      esp32Tools.cpp
   @date      20240121
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

// This is a collection of ESP32 specific low-level functions.

#if defined(ARDUINO_ARCH_ESP32)

#include <Arduino.h>
#include "esp32Tools.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <Esp.h>
// get the right RTC.H for each MCU
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
#if CONFIG_IDF_TARGET_ESP32S2
#include <esp32s2/rom/rtc.h>
#elif CONFIG_IDF_TARGET_ESP32C3
#include <esp32c3/rom/rtc.h>
#elif CONFIG_IDF_TARGET_ESP32S3
#include <esp32s3/rom/rtc.h>
#elif CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
#include <esp32/rom/rtc.h>
#endif
#else // ESP32 Before IDF 4.0
#include <rom/rtc.h>
#endif

// forward declarations of local helper functions
static esp_reset_reason_t getRestartReason();
static int getCoreResetReason(int core);
static String resetCode2Info(int reason);
static String restartCode2InfoLong(esp_reset_reason_t reason);
static String restartCode2Info(esp_reset_reason_t reason);


// check if estart was "normal"
bool sysTools_normal_startup() {
	esp_reset_reason_t restartCode = getRestartReason();
	if ((restartCode == ESP_RST_POWERON) || (restartCode == ESP_RST_SW)) return true;  // poweron or esp_restart()
	return false;
}

// RESTART reason as long string
String sysTools_getRestartReason() {
  esp_reset_reason_t restartCode = getRestartReason();
	String reasonText = restartCode2InfoLong(restartCode);
	String longText = String("(code ") + String((int)restartCode) + String( ") ") + reasonText;

  int core0code = getCoreResetReason(0);
  int core1code = getCoreResetReason(1);
  longText = longText + ". Core#0 (code " + String(core0code) + ") " + resetCode2Info(core0code);
  if (core1code > 0) 
    longText = longText + "; Core#1 (code " + String(core1code) + ") " + resetCode2Info(core1code);

  longText = longText + ".";
  return longText;
}
	




// stack debug tools - find loopTask task, and queries it's free stack size

#if INCLUDE_xTaskGetHandle   // only supported on newer frameworks
int sysTools_get_arduino_maxStackUsage(void) {
  static TaskHandle_t loop_taskHandle = NULL;                   // to store the task handle for later calls
  char * loop_taskname = pcTaskGetTaskName(loop_taskHandle);    // ask for name of the known task (to make sure we are still looking at the right one)

  if ((loop_taskHandle == NULL) || (loop_taskname == NULL) || (strncmp(loop_taskname, "loopTask", 8) != 0)) {
    loop_taskHandle = xTaskGetHandle("loopTask");              // need to look for the task by name. FreeRTOS docs say this is very slow, so we store the result for next time
  }

  if (loop_taskHandle != NULL) return uxTaskGetStackHighWaterMark(loop_taskHandle); // got it !!
  else return -1;
}

int sysTools_get_webserver_maxStackUsage(void) {
  static TaskHandle_t tcp_taskHandle = NULL;                   // to store the task handle for later calls
  char * tcp_taskname = pcTaskGetTaskName(tcp_taskHandle);     // ask for name of the known task (to make sure we are still looking at the right one)

  if ((tcp_taskHandle == NULL) || (tcp_taskname == NULL) || (strncmp(tcp_taskname, "async_tcp", 9) != 0)) {
    tcp_taskHandle = xTaskGetHandle("async_tcp");              // need to look for the task by name. FreeRTOS docs say this is very slow, so we store the result for next time
  }

  if (tcp_taskHandle != NULL) return uxTaskGetStackHighWaterMark(tcp_taskHandle); // got it !!
  else return -1;
}
#else
#warning cannot query task stacksize on your system
int sysTools_get_arduino_maxStackUsage(void) { return -1;}
int sysTools_get_webserver_maxStackUsage(void)  { return -1;}
#endif


// helper fuctions
static int getCoreResetReason(int core) {
  if (core >= ESP.getChipCores()) return 0;
  return((int)rtc_get_reset_reason(core));
}

static String resetCode2Info(int reason) {
  switch(reason) {

    case 1 : //  1 =  Vbat power on reset
      return F("power-on"); break;
    case 2 : // 2 = this code is not defined on ESP32
      return F("exception"); break;
    case 3 : // 3 = Software reset digital core
       return F("SW reset"); break;
    case 12: //12 = Software reset CPU
       return F("SW restart"); break;
    case 5 : // 5 = Deep Sleep wakeup reset digital core
       return F("wakeup"); break;
    case 14:  //14 = for APP CPU, reset by PRO CPU
      return F("restart"); break;
    case 15: //15 = Reset when the vdd voltage is not stable (brownout)
      return F("brown-out"); break;

    // watchdog resets
    case 4 : // 4 = Legacy watch dog reset digital core
    case 6 : // 6 = Reset by SLC module, reset digital core
    case 7 : // 7 = Timer Group0 Watch dog reset digital core
    case 8 : // 8 = Timer Group1 Watch dog reset digital core
    case 9 : // 9 = RTC Watch dog Reset digital core
    case 11: //11 = Time Group watchdog reset CPU
    case 13: //13 = RTC Watch dog Reset CPU
    case 16: //16 = RTC Watch dog reset digital core and rtc module
    case 17: //17 = Time Group1 reset CPU
      return F("watchdog"); break;
    case 18: //18 = super watchdog reset digital core and rtc module
      return F("super watchdog"); break;

    // misc
    case 10: // 10 = Instrusion tested to reset CPU
      return F("intrusion"); break;
    case 19: //19 = glitch reset digital core and rtc module
      return F("glitch"); break;
    case 20: //20 = efuse reset digital core
      return F("EFUSE reset"); break;
    case 21: //21 = usb uart reset digital core
      return F("USB UART reset"); break;
    case 22: //22 = usb jtag reset digital core
     return F("JTAG reset"); break;
    case 23: //23 = power glitch reset digital core and rtc module
      return F("power glitch"); break;

    // unknown reason code
    case 0:
      return F(""); break;
    default: 
      return F("unknown"); break;
  }
}


static esp_reset_reason_t getRestartReason() {
  return(esp_reset_reason());
}

static String restartCode2InfoLong(esp_reset_reason_t reason) {
    switch (reason) {
      case ESP_RST_UNKNOWN:  return(F("Reset reason can not be determined")); break;
      case ESP_RST_POWERON:  return(F("Restart due to power-on event")); break;
      case ESP_RST_EXT:      return(F("Reset by external pin (not applicable for ESP32)")); break;
      case ESP_RST_SW:       return(F("Software restart via esp_restart()")); break;
      case ESP_RST_PANIC:    return(F("Software reset due to panic or unhandled exception (SW error)")); break;
      case ESP_RST_INT_WDT:  return(F("Reset (software or hardware) due to interrupt watchdog")); break;
      case ESP_RST_TASK_WDT: return(F("Reset due to task watchdog")); break;
      case ESP_RST_WDT:      return(F("Reset due to other watchdogs")); break;
      case ESP_RST_DEEPSLEEP:return(F("Restart after exiting deep sleep mode")); break;
      case ESP_RST_BROWNOUT: return(F("Brownout Reset (software or hardware)")); break;
      case ESP_RST_SDIO:     return(F("Reset over SDIO")); break;
    }
  return(F("unknown"));
}

static String restartCode2Info(esp_reset_reason_t reason) {
    switch (reason) {
      case ESP_RST_UNKNOWN:  return(F("unknown reason")); break;
      case ESP_RST_POWERON:  return(F("power-on event")); break;
      case ESP_RST_EXT:      return(F("external pin reset")); break;
      case ESP_RST_SW:       return(F("SW restart by esp_restart()")); break;
      case ESP_RST_PANIC:    return(F("SW error (panic or exception)")); break;
      case ESP_RST_INT_WDT:  return(F("interrupt watchdog")); break;
      case ESP_RST_TASK_WDT: return(F("task watchdog")); break;
      case ESP_RST_WDT:      return(F("other watchdog")); break;
      case ESP_RST_DEEPSLEEP:return(F("exit from deep sleep")); break;
      case ESP_RST_BROWNOUT: return(F("Brownout Reset")); break;
      case ESP_RST_SDIO:     return(F("Reset over SDIO")); break;
    }
  return(F("unknown"));
}


// from WLEDMM - will integrate this later
#if 0
  DEBUG_PRINT(F("esp32 "));
  DEBUG_PRINTLN(ESP.getSdkVersion());
  #if defined(ESP_ARDUINO_VERSION)
    DEBUG_PRINTF("arduino-esp32 v%d.%d.%d\n", int(ESP_ARDUINO_VERSION_MAJOR), int(ESP_ARDUINO_VERSION_MINOR), int(ESP_ARDUINO_VERSION_PATCH));  // availeable since v2.0.0
  #else
    DEBUG_PRINTLN(F("arduino-esp32 v1.0.x\n"));  // we can't say in more detail.
  #endif

  USER_PRINT(F("CPU:   ")); USER_PRINT(ESP.getChipModel());
  USER_PRINT(F(" rev.")); USER_PRINT(ESP.getChipRevision());
  USER_PRINT(F(", ")); USER_PRINT(ESP.getChipCores()); USER_PRINT(F(" core(s)"));
  USER_PRINT(F(", ")); USER_PRINT(ESP.getCpuFreqMHz()); USER_PRINTLN(F("MHz."));

  USER_PRINT(F("CPU    "));
  esp_reset_reason_t resetReason = getRestartReason();
  USER_PRINT(restartCode2InfoLong(resetReason));
  USER_PRINT(F(" (code "));
  USER_PRINT((int)resetReason);
  USER_PRINT(F("). "));
  int core0code = getCoreResetReason(0);
  int core1code = getCoreResetReason(1);
  USER_PRINTF("Core#0 %s (%d)", resetCode2Info(core0code).c_str(), core0code);
  if (core1code > 0) {USER_PRINTF("; Core#1 %s (%d)", resetCode2Info(core1code).c_str(), core1code);}
  USER_PRINTLN(F("."));

  USER_PRINT(F("FLASH: ")); USER_PRINT((ESP.getFlashChipSize()/1024)/1024);
  USER_PRINT(F("MB, Mode ")); USER_PRINT(ESP.getFlashChipMode());
  switch (ESP.getFlashChipMode()) {
    // missing: Octal modes
    case FM_QIO:  DEBUG_PRINT(F(" (QIO)")); break;
    case FM_QOUT: DEBUG_PRINT(F(" (QOUT)"));break;
    case FM_DIO:  DEBUG_PRINT(F(" (DIO)")); break;
    case FM_DOUT: DEBUG_PRINT(F(" (DOUT)"));break;
    default: break;
  }
  USER_PRINT(F(", speed ")); USER_PRINT(ESP.getFlashChipSpeed()/1000000);USER_PRINTLN(F("MHz."));  
#endif



#else
	#error SystemTools are not yet availeable for your environment
#endif