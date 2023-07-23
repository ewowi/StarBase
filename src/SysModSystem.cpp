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

  parentObject = ui->initGroup(parentObject, name);

  ui->initDisplay(parentObject, "upTime", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "comment", "Uptime of board");
  });
  ui->initDisplay(parentObject, "loops");
  ui->initDisplay(parentObject, "heap", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "comment", "Free / Total (largest free)");
  });
  ui->initDisplay(parentObject, "stack");

  ui->initButton(parentObject, "restart", "Restart", nullptr, [](JsonObject object) {  //chFun
    web->ws->closeAll(1012);
    ESP.restart();
  });

  ui->initDropdown(parentObject, "reset0", (int)rtc_get_reset_reason(0), [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "comment", "Reset reason core 0 (to do readonly)");
    sys->addResetReasonsLov(web->addResponseA(object["id"], "lov"));
  });
  if (ESP.getChipCores() > 1)
    ui->initDropdown(parentObject, "reset1", (int)rtc_get_reset_reason(1), [](JsonObject object) { //uiFun
      web->addResponse(object["id"], "comment", "Reset reason core 1 (to do readonly)");
      sys->addResetReasonsLov(web->addResponseA(object["id"], "lov"));
    });
  ui->initDropdown(parentObject, "restartReason", (int)esp_reset_reason(), [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "comment", "Restart reason (to do readonly)");
    JsonArray lov = web->addResponseA(object["id"], "lov");
    lov.add("ESP_RST_UNKNOWN");//  //!< Reset reason can not be determined
    lov.add("ESP_RST_POWERON");//  //!< Reset due to power-on event
    lov.add("ESP_RST_EXT");//      //!< Reset by external pin (not applicable for ESP32)
    lov.add("ESP_RST_SW");//       //!< Software reset via esp_restart
    lov.add("ESP_RST_PANIC");//    //!< Software reset due to exception/panic
    lov.add("ESP_RST_INT_WDT");//  //!< Reset (software or hardware) due to interrupt watchdog
    lov.add("ESP_RST_TASK_WDT");// //!< Reset due to task watchdog
    lov.add("ESP_RST_WDT");//      //!< Reset due to other watchdogs
    lov.add("ESP_RST_DEEPSLEEP");////!< Reset after exiting deep sleep mode
    lov.add("ESP_RST_BROWNOUT");// //!< Brownout reset (software or hardware)
    lov.add("ESP_RST_SDIO");//     //!< Reset over SDIO

  });

  // static char msgbuf[32];
  // snprintf(msgbuf, sizeof(msgbuf)-1, "%s rev.%d", ESP.getChipModel(), ESP.getChipRevision());
  // ui->initDisplay(parentObject, "e32model")] = msgbuf;
  // ui->initDisplay(parentObject, "e32cores")] = ESP.getChipCores();
  // ui->initDisplay(parentObject, "e32speed")] = ESP.getCpuFreqMHz();
  // ui->initDisplay(parentObject, "e32flash")] = int((ESP.getFlashChipSize()/1024)/1024);
  // ui->initDisplay(parentObject, "e32flashspeed")] = int(ESP.getFlashChipSpeed()/1000000);
  // ui->initDisplay(parentObject, "e32flashmode")] = int(ESP.getFlashChipMode());
  // switch (ESP.getFlashChipMode()) {
  //   // missing: Octal modes
  //   case FM_QIO:  ui->initDisplay(parentObject, "e32flashtext")] = F(" (QIO)"); break;
  //   case FM_QOUT: ui->initDisplay(parentObject, "e32flashtext")] = F(" (QOUT)");break;
  //   case FM_DIO:  ui->initDisplay(parentObject, "e32flashtext")] = F(" (DIO)"); break;
  //   case FM_DOUT: ui->initDisplay(parentObject, "e32flashtext")] = F(" (DOUT or other)");break;
  //   default: ui->initDisplay(parentObject, "e32flashtext")] = F(" (other)"); break;
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

void SysModSystem::addResetReasonsLov(JsonArray lov) {
    lov.add("NO_MEAN"); // 0,
    lov.add("POWERON_RESET"); // 1,    /**<1, Vbat power on reset*/
    lov.add("SW_RESET"); // 3,    /**<3, Software reset digital core*/
    lov.add("OWDT_RESET"); // 4,    /**<4, Legacy watch dog reset digital core*/
    lov.add("DEEPSLEEP_RESET"); // 5,    /**<3, Deep Sleep reset digital core*/
    lov.add("SDIO_RESET"); // 6,    /**<6, Reset by SLC module, reset digital core*/
    lov.add("TG0WDT_SYS_RESET"); // 7,    /**<7, Timer Group0 Watch dog reset digital core*/
    lov.add("TG1WDT_SYS_RESET"); // 8,    /**<8, Timer Group1 Watch dog reset digital core*/
    lov.add("RTCWDT_SYS_RESET"); // 9,    /**<9, RTC Watch dog Reset digital core*/
    lov.add("INTRUSION_RESET"); //10,    /**<10, Instrusion tested to reset CPU*/
    lov.add("TGWDT_CPU_RESET"); //11,    /**<11, Time Group reset CPU*/
    lov.add("SW_CPU_RESET"); //12,    /**<12, Software reset CPU*/
    lov.add("RTCWDT_CPU_RESET"); //13,    /**<13, RTC Watch dog Reset CPU*/
    lov.add("EXT_CPU_RESET"); //14,    /**<14, for APP CPU, reseted by PRO CPU*/
    lov.add("RTCWDT_BROWN_OUT_RESET"); //15,    /**<15, Reset when the vdd voltage is not stable*/
    lov.add("RTCWDT_RTC_RESET"); //16     /**<16, RTC Watch dog reset digital core and rtc module*/
}
