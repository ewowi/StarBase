#include "SysModSystem.h"
#include "Module.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModModel.h"

// #include <Esp.h>
#include <rom/rtc.h>

SysModSystem::SysModSystem() :Module("System") {};

void SysModSystem::setup() {
  Module::setup();
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentObject = ui->initModule(parentObject, name);

  ui->initText(parentObject, "upTime", nullptr, true, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "comment", "Uptime of board");
  });
  ui->initText(parentObject, "loops");
  ui->initText(parentObject, "heap", nullptr, true, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "comment", "Free / Total (largest free)");
  });
  ui->initText(parentObject, "stack");

  ui->initButton(parentObject, "restart", nullptr, nullptr, [](JsonObject object) {  //chFun
    web->ws->closeAll(1012);
    ESP.restart();
  });

  ui->initSelect(parentObject, "reset0", (int)rtc_get_reset_reason(0), true, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "Reset 0");
    web->addResponse(object["id"], "comment", "Reason Core 0");
    sys->addResetReasonsSelect(web->addResponseA(object["id"], "select"));
  });
  if (ESP.getChipCores() > 1)
    ui->initSelect(parentObject, "reset1", (int)rtc_get_reset_reason(1), true, [](JsonObject object) { //uiFun
      web->addResponse(object["id"], "label", "Reset 1");
      web->addResponse(object["id"], "comment", "Reason Core 1");
      sys->addResetReasonsSelect(web->addResponseA(object["id"], "select"));
    });
  ui->initSelect(parentObject, "restartReason", (int)esp_reset_reason(), true, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "Restart");
    web->addResponse(object["id"], "comment", "Reason restart");
    sys->addRestartReasonsSelect(web->addResponseA(object["id"], "select"));
  });

  // static char msgbuf[32];
  // snprintf(msgbuf, sizeof(msgbuf)-1, "%s rev.%d", ESP.getChipModel(), ESP.getChipRevision());
  // ui->initText(parentObject, "e32model")] = msgbuf;
  // ui->initText(parentObject, "e32cores")] = ESP.getChipCores();
  // ui->initText(parentObject, "e32speed")] = ESP.getCpuFreqMHz();
  // ui->initText(parentObject, "e32flash")] = int((ESP.getFlashChipSize()/1024)/1024);
  // ui->initText(parentObject, "e32flashspeed")] = int(ESP.getFlashChipSpeed()/1000000);
  // ui->initText(parentObject, "e32flashmode")] = int(ESP.getFlashChipMode());
  // switch (ESP.getFlashChipMode()) {
  //   // missing: Octal modes
  //   case FM_QIO:  ui->initText(parentObject, "e32flashtext")] = F(" (QIO)"); break;
  //   case FM_QOUT: ui->initText(parentObject, "e32flashtext")] = F(" (QOUT)");break;
  //   case FM_DIO:  ui->initText(parentObject, "e32flashtext")] = F(" (DIO)"); break;
  //   case FM_DOUT: ui->initText(parentObject, "e32flashtext")] = F(" (DOUT or other)");break;
  //   default: ui->initText(parentObject, "e32flashtext")] = F(" (other)"); break;
  // }

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModSystem::loop() {
  // Module::loop();

  loopCounter++;
  if (millis() - secondMillis >= 1000 || !secondMillis) {
    secondMillis = millis();

    mdl->setValueV("upTime", "%u s", millis()/1000);
    mdl->setValueV("loops", "%lu /s", loopCounter);
    mdl->setValueV("heap", "%d / %d (%d) B", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
    mdl->setValueV("stack", "%d B", uxTaskGetStackHighWaterMark(NULL));

    loopCounter = 0;
  }
}

//replace code by sentence as soon it occurs, so we know what will happen and what not
void SysModSystem::addResetReasonsSelect(JsonArray select) {
  select.add("NO_MEAN"); // 0,
  select.add("POWERON_RESET"); // 1,    /**<1, Vbat power on reset*/
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
  select.add("EXT_CPU_RESET"); //14,    /**<14, for APP CPU, reseted by PRO CPU*/
  select.add("RTCWDT_BROWN_OUT_RESET"); //15,    /**<15, Reset when the vdd voltage is not stable*/
  select.add("RTCWDT_RTC_RESET"); //16     /**<16, RTC Watch dog reset digital core and rtc module*/
}

//replace code by sentence as soon it occurs, so we know what will happen and what not
void SysModSystem::addRestartReasonsSelect(JsonArray select) {
  select.add("ESP_RST_UNKNOWN");//  //!< Reset reason can not be determined
  select.add("ESP_RST_POWERON");//  //!< Reset due to power-on event
  select.add("ESP_RST_EXT");//      //!< Reset by external pin (not applicable for ESP32)
  select.add("ESP_RST_SW");//       //!< Software reset via esp_restart
  select.add("SW reset due to exception/panic (4)");//ESP_RST_PANIC");//    //!< 
  select.add("ESP_RST_INT_WDT");//  //!< Reset (software or hardware) due to interrupt watchdog
  select.add("ESP_RST_TASK_WDT");// //!< Reset due to task watchdog
  select.add("ESP_RST_WDT");//      //!< Reset due to other watchdogs
  select.add("ESP_RST_DEEPSLEEP");////!< Reset after exiting deep sleep mode
  select.add("ESP_RST_BROWNOUT");// //!< Brownout reset (software or hardware)
  select.add("ESP_RST_SDIO");//     //!< Reset over SDIO
}