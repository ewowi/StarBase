/*
   @title     StarMod
   @file      SysModSystem.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModSystem.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModModel.h"
#include "esp32Tools.h"

// #include <Esp.h>
#include <rom/rtc.h>

SysModSystem::SysModSystem() :SysModule("System") {};

void SysModSystem::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name);
  if (parentVar["o"] > -1000) parentVar["o"] = -2100; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

  ui->initText(parentVar, "serverName", "StarMod", UINT8_MAX, 32, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
    web->addResponse(var["id"], "comment", "Instance name");
  });
  ui->initText(parentVar, "upTime", nullptr, UINT8_MAX, 16, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Uptime of board");
  });
  ui->initText(parentVar, "loops", nullptr, UINT8_MAX, 16, true);

  ui->initText(parentVar, "chip", nullptr, UINT8_MAX, 16, true);

  ui->initProgress(parentVar, "heap", 0, UINT8_MAX, 0, ESP.getHeapSize()/1000, true, nullptr
  , [](JsonObject var, uint8_t) {  //chFun
    var["max"] = ESP.getHeapSize()/1000; //makes sense?
    web->addResponse(var["id"], "value", (ESP.getHeapSize()-ESP.getFreeHeap()) / 1000);
    web->addResponseV(var["id"], "comment", "f:%d / t:%d (l:%d) B", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
  });

  if (psramFound()) {
    ui->initProgress(parentVar, "psram", 0, UINT8_MAX, 0, ESP.getPsramSize()/1000, true, nullptr
    , [](JsonObject var, uint8_t) {  //chFun
      var["max"] = ESP.getPsramSize()/1000; //makes sense?
      web->addResponse(var["id"], "value", (ESP.getPsramSize()-ESP.getFreePsram()) / 1000);
      web->addResponseV(var["id"], "comment", "%d / %d (%d) B", ESP.getFreePsram(), ESP.getPsramSize(), ESP.getMinFreePsram());
    });
  }

  ui->initProgress(parentVar, "stack", 0, UINT8_MAX, 0, 4096, true, nullptr
  , [](JsonObject var, uint8_t) {  //chFun
    web->addResponse(var["id"], "value", uxTaskGetStackHighWaterMark(NULL));
    web->addResponseV(var["id"], "comment", "%d B", uxTaskGetStackHighWaterMark(NULL));
  });

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

  ui->initSelect(parentVar, "reset0", (int)rtc_get_reset_reason(0), UINT8_MAX, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Reset 0");
    web->addResponse(var["id"], "comment", "Reason Core 0");
    sys->addResetReasonsSelect(web->addResponseA(var["id"], "options"));
  });
  if (ESP.getChipCores() > 1)
    ui->initSelect(parentVar, "reset1", (int)rtc_get_reset_reason(1), UINT8_MAX, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Reset 1");
      web->addResponse(var["id"], "comment", "Reason Core 1");
      sys->addResetReasonsSelect(web->addResponseA(var["id"], "options"));
    });
  ui->initSelect(parentVar, "restartReason", (int)esp_reset_reason(), UINT8_MAX, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Restart");
    web->addResponse(var["id"], "comment", "Reason restart");
    sys->addRestartReasonsSelect(web->addResponseA(var["id"], "options"));
  });

  //calculate version in format YYMMDDHH
  //https://forum.arduino.cc/t/can-you-format-__date__/200818/10
  int month, day, year, hour, minute, second;
  const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  sscanf(__DATE__, "%s %d %d", version, &day, &year); // Mon dd yyyy
  month = (strstr(month_names, version)-month_names)/3+1;
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second); //hh:mm:ss
  print->fFormat(version, sizeof(version)-1, "%02d%02d%02d%02d", year-2000, month, day, hour);

  USER_PRINTF("version %s %s %s %d:%d:%d\n", version, __DATE__, __TIME__, hour, minute, second);

  ui->initText(parentVar, "version", nullptr, UINT8_MAX, 16, true);
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
}

void SysModSystem::loop() {
  // SysModule::loop();

  loopCounter++;
}
void SysModSystem::loop1s() {
  mdl->setUIValueV("upTime", "%lu s", millis()/1000);
  mdl->setUIValueV("loops", "%lu /s", loopCounter);

  loopCounter = 0;
}
void SysModSystem::loop10s() {
  mdl->setValue("version", JsonString(version)); //make sure ui shows the right version !!!never do this in uiFun as it interupts with uiFun sendDataWS!!

  mdl->setUIValueV("chip", "%s %s c#:%d %d mHz f:%d KB %d mHz %d", ESP.getChipModel(), ESP.getSdkVersion(), ESP.getChipCores(), ESP.getCpuFreqMHz(), ESP.getFlashChipSize()/1024, ESP.getFlashChipSpeed()/1000000, ESP.getFlashChipMode());

  mdl->callChFunAndWs(mdl->findVar("heap"));

  mdl->callChFunAndWs(mdl->findVar("stack"));

  if (psramFound()) {
    // mdl->setUIValueV("psram", "%d / %d (%d) B", ESP.getFreePsram(), ESP.getPsramSize(), ESP.getMinFreePsram());
    mdl->callChFunAndWs(mdl->findVar("psram"));
  }
  USER_PRINTF("❤️"); //heartbeat
}

//replace code by sentence as soon it occurs, so we know what will happen and what not
void SysModSystem::addResetReasonsSelect(JsonArray select) {
  select.add(String("NO_MEAN (0)")); // 0,
  select.add(sysTools_reset2String( 1)); //POWERON_RESET"); // 1,    /**<1, */
  select.add(sysTools_reset2String( 2)); // 2,    /**<3, Software reset digital core*/
  select.add(sysTools_reset2String( 3)); // 3,    /**<3, Software reset digital core*/
  select.add(sysTools_reset2String( 4)); // 4,    /**<4, Legacy watch dog reset digital core*/
  select.add(sysTools_reset2String( 5)); // 5,    /**<3, Deep Sleep reset digital core*/
  select.add(sysTools_reset2String( 6)); // 6,    /**<6, Reset by SLC module, reset digital core*/
  select.add(sysTools_reset2String( 7)); // 7,    /**<7, Timer Group0 Watch dog reset digital core*/
  select.add(sysTools_reset2String( 8)); // 8,    /**<8, Timer Group1 Watch dog reset digital core*/
  select.add(sysTools_reset2String( 9)); // 9,    /**<9, RTC Watch dog Reset digital core*/
  select.add(sysTools_reset2String(10)); //10,    /**<10, Instrusion tested to reset CPU*/
  select.add(sysTools_reset2String(11)); //11,    /**<11, Time Group reset CPU*/
  select.add(sysTools_reset2String(12)); //SW_CPU_RESET"); //12,    /**<12, */
  select.add(sysTools_reset2String(13)); //13,    /**<13, RTC Watch dog Reset CPU*/
  select.add(sysTools_reset2String(14)); //EXT_CPU_RESET"); //14,    /**<14, */
  select.add(sysTools_reset2String(15)); //15,    /**<15, Reset when the vdd voltage is not stable*/
  select.add(sysTools_reset2String(16)); //16     /**<16, RTC Watch dog reset digital core and rtc module*/
  // codes below are only used on -S3/-S2/-C3
  select.add(sysTools_reset2String(17)); //17     /* Time Group1 reset CPU */
  select.add(sysTools_reset2String(18)); //18     /* super watchdog reset digital core and rtc module */
  select.add(sysTools_reset2String(19)); //19     /* glitch reset digital core and rtc module */
  select.add(sysTools_reset2String(20)); //20     /* efuse reset digital core */
  select.add(sysTools_reset2String(21)); //21     /* usb uart reset digital core */
  select.add(sysTools_reset2String(22)); //22     /* usb jtag reset digital core */
  select.add(sysTools_reset2String(23)); //23     /* power glitch reset digital core and rtc module */
}

//replace code by sentence as soon it occurs, so we know what will happen and what not
void SysModSystem::addRestartReasonsSelect(JsonArray select) {
  select.add(String("(0) ESP_RST_UNKNOWN"));//           //!< Reset reason can not be determined
  select.add(sysTools_restart2String( 1)); // ESP_RST_POWERON");//  //!< 
  select.add(sysTools_restart2String( 2)); // !< Reset by external pin (not applicable for ESP32)
  select.add(sysTools_restart2String( 3)); // ESP_RST_SW");//       //!< Software reset via esp_restart
  select.add(sysTools_restart2String( 4)); //ESP_RST_PANIC");//    //!< 
  select.add(sysTools_restart2String( 5)); //  //!< Reset (software or hardware) due to interrupt watchdog
  select.add(sysTools_restart2String( 6)); // //!< Reset due to task watchdog
  select.add(sysTools_restart2String( 7)); //      //!< Reset due to other watchdogs
  select.add(sysTools_restart2String( 8)); ////!< Reset after exiting deep sleep mode
  select.add(sysTools_restart2String( 9)); // //!< Brownout reset (software or hardware)
  select.add(sysTools_restart2String(10)); //     //!< Reset over SDIO
}