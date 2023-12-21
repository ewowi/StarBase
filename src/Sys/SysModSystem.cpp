/*
   @title     StarMod
   @file      SysModSystem.cpp
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "SysModSystem.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModModel.h"

// #include <Esp.h>
#include <rom/rtc.h>

SysModSystem::SysModSystem() :SysModule("System") {};

void SysModSystem::setup() {
  SysModule::setup();
  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = ui->initModule(parentVar, name);

  ui->initText(parentVar, "serverName", "StarMod", 32, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
    web->addResponse(var["id"], "comment", "Instance name");
  });
  ui->initText(parentVar, "upTime", nullptr, 16, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Uptime of board");
  });
  ui->initText(parentVar, "loops", nullptr, 16, true);
  ui->initText(parentVar, "heap", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Free / Total (largest free)");
  });
  ui->initText(parentVar, "stack", nullptr, 16, true);

  ui->initButton(parentVar, "reboot", nullptr, false, nullptr, [](JsonObject var, uint8_t) {  //chFun
    web->ws->closeAll(1012);

    // mdls->reboot(); //not working yet
    // long dly = millis();
    // while (millis() - dly < 450) {
    //   yield();        // enough time to send response to client
    // }
    // FASTLED.clear();
    ESP.restart();
  });

  ui->initSelect(parentVar, "reset0", (int)rtc_get_reset_reason(0), true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Reset 0");
    web->addResponse(var["id"], "comment", "Reason Core 0");
    sys->addResetReasonsSelect(web->addResponseA(var["id"], "data"));
  });
  if (ESP.getChipCores() > 1)
    ui->initSelect(parentVar, "reset1", (int)rtc_get_reset_reason(1), true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Reset 1");
      web->addResponse(var["id"], "comment", "Reason Core 1");
      sys->addResetReasonsSelect(web->addResponseA(var["id"], "data"));
    });
  ui->initSelect(parentVar, "restartReason", (int)esp_reset_reason(), true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Restart");
    web->addResponse(var["id"], "comment", "Reason restart");
    sys->addRestartReasonsSelect(web->addResponseA(var["id"], "data"));
  });

  //calculate version in format YYMMDDHH
  //https://forum.arduino.cc/t/can-you-format-__date__/200818/10
  int month, day, year, hour, minute, second;
  const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  sscanf(__DATE__, "%s %d %d", version, &day, &year); // Oct 20 2023
  month = (strstr(month_names, version)-month_names)/3+1;
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second); //11:20:23
  print->fFormat(version, sizeof(version)-1, "%02d%02d%02d%02d", year-2000, month, day, hour);

  USER_PRINTF("version %s %s %s %d:%d:%d\n", version, __DATE__, __TIME__, hour, minute, second);

  ui->initText(parentVar, "version", nullptr, 16, true);
  // ui->initText(parentVar, "date", __DATE__, 16, true);
  // ui->initText(parentVar, "time", __TIME__, 16, true);


  // static char msgbuf[32];
  // snprintf(msgbuf, sizeof(msgbuf)-1, "%s rev.%d", ESP.getChipModel(), ESP.getChipRevision());
  // ui->initText(parentVar, "e32model")] = msgbuf;
  // ui->initText(parentVar, "e32cores")] = ESP.getChipCores();
  // ui->initText(parentVar, "e32speed")] = ESP.getCpuFreqMHz();
  // ui->initText(parentVar, "e32flash")] = int((ESP.getFlashChipSize()/1024)/1024);
  // ui->initText(parentVar, "e32flashspeed")] = int(ESP.getFlashChipSpeed()/1000000);
  // ui->initText(parentVar, "e32flashmode")] = int(ESP.getFlashChipMode());
  // switch (ESP.getFlashChipMode()) {
  //   // missing: Octal modes
  //   case FM_QIO:  ui->initText(parentVar, "e32flashtext")] = F(" (QIO)"); break;
  //   case FM_QOUT: ui->initText(parentVar, "e32flashtext")] = F(" (QOUT)");break;
  //   case FM_DIO:  ui->initText(parentVar, "e32flashtext")] = F(" (DIO)"); break;
  //   case FM_DOUT: ui->initText(parentVar, "e32flashtext")] = F(" (DOUT or other)");break;
  //   default: ui->initText(parentVar, "e32flashtext")] = F(" (other)"); break;
  // }

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModSystem::loop() {
  // SysModule::loop();

  loopCounter++;
}
void SysModSystem::loop1s() {
  mdl->setValueLossy("upTime", "%lu s", millis()/1000);
  mdl->setValueLossy("loops", "%lu /s", loopCounter);

  loopCounter = 0;
}
void SysModSystem::loop10s() {
  mdl->setValueC("version", version); //make sure ui shows the right version !!!never do this as it interupts with uiFun sendDataWS!!
  mdl->setValueLossy("heap", "%d / %d (%d) B", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
  mdl->setValueLossy("stack", "%d B", uxTaskGetStackHighWaterMark(NULL));
  USER_PRINTF("❤️"); //heartbeat
}

//replace code by sentence as soon it occurs, so we know what will happen and what not
void SysModSystem::addResetReasonsSelect(JsonArray select) {
  select.add("NO_MEAN"); // 0,
  select.add("Vbat power on reset");//POWERON_RESET"); // 1,    /**<1, */
  select.add("SW_RESET (2)"); // 2,    /**<3, Software reset digital core*/
  select.add("SW_RESET (3)"); // 3,    /**<3, Software reset digital core*/
  select.add("OWDT_RESET"); // 4,    /**<4, Legacy watch dog reset digital core*/
  select.add("DEEPSLEEP_RESET"); // 5,    /**<3, Deep Sleep reset digital core*/
  select.add("SDIO_RESET"); // 6,    /**<6, Reset by SLC module, reset digital core*/
  select.add("TG0WDT_SYS_RESET"); // 7,    /**<7, Timer Group0 Watch dog reset digital core*/
  select.add("TG1WDT_SYS_RESET"); // 8,    /**<8, Timer Group1 Watch dog reset digital core*/
  select.add("RTCWDT_SYS_RESET"); // 9,    /**<9, RTC Watch dog Reset digital core*/
  select.add("INTRUSION_RESET"); //10,    /**<10, Instrusion tested to reset CPU*/
  select.add("TGWDT_CPU_RESET"); //11,    /**<11, Time Group reset CPU*/
  select.add("SW reset CPU (12)");//SW_CPU_RESET"); //12,    /**<12, */
  select.add("RTCWDT_CPU_RESET"); //13,    /**<13, RTC Watch dog Reset CPU*/
  select.add("for APP CPU, reset by PRO CPU");//EXT_CPU_RESET"); //14,    /**<14, */
  select.add("RTCWDT_BROWN_OUT_RESET"); //15,    /**<15, Reset when the vdd voltage is not stable*/
  select.add("RTCWDT_RTC_RESET"); //16     /**<16, RTC Watch dog reset digital core and rtc module*/
}

//replace code by sentence as soon it occurs, so we know what will happen and what not
void SysModSystem::addRestartReasonsSelect(JsonArray select) {
  select.add("ESP_RST_UNKNOWN");//  //!< Reset reason can not be determined
  select.add("Reset due to power-on event");//ESP_RST_POWERON");//  //!< 
  select.add("ESP_RST_EXT");//      //!< Reset by external pin (not applicable for ESP32)
  select.add("Software reset via esp_restart (3)");//ESP_RST_SW");//       //!< Software reset via esp_restart
  select.add("SW reset due to exception/panic (4)");//ESP_RST_PANIC");//    //!< 
  select.add("ESP_RST_INT_WDT");//  //!< Reset (software or hardware) due to interrupt watchdog
  select.add("ESP_RST_TASK_WDT");// //!< Reset due to task watchdog
  select.add("ESP_RST_WDT");//      //!< Reset due to other watchdogs
  select.add("ESP_RST_DEEPSLEEP");////!< Reset after exiting deep sleep mode
  select.add("ESP_RST_BROWNOUT");// //!< Brownout reset (software or hardware)
  select.add("ESP_RST_SDIO");//     //!< Reset over SDIO
}