/*
   @title     StarMod
   @file      SysModules.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModules.h"
#include "Sys/SysModPrint.h"
#include "Sys/SysModUI.h"
#include "Sys/SysModWeb.h"
#include "Sys/SysModModel.h"

bool SysModules::newConnection = false;
bool SysModules::isConnected = false;

SysModules::SysModules() {
  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

void SysModules::setup() {
  for (SysModule *module:modules) {
    module->setup();
  }

  //do its own setup: will be shown as last module
  JsonObject parentVar;
  parentVar = ui->initSysMod(parentVar, "Modules");

  JsonObject tableVar = ui->initTable(parentVar, "mdlTbl", nullptr, true, [this](JsonObject var) { //uiFun ro true: no update and delete
    web->addResponse(var["id"], "label", "Modules");
    web->addResponse(var["id"], "comment", "List of modules");
  });

  ui->initText(tableVar, "mdlName", nullptr, 32, true, [this](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
  }, nullptr, nullptr, modules.size(), [this](JsonObject var, uint8_t rowNr) { //valueFun
    var["value"][rowNr] = modules[rowNr]->name;
  });

  ui->initCheckBox(tableVar, "mdlSucces", uint16Max, true, [this](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Success");
  }, nullptr, nullptr, modules.size(), [this](JsonObject var, uint8_t rowNr) { //valueFun
    var["value"][rowNr] = modules[rowNr]->success;
  });

  ui->initCheckBox(tableVar, "mdlEnabled", uint16Max, false, [this](JsonObject var) { //uiFun not readonly! (tbd)
    //initially set to true, but as enabled are table cells, will be updated to an array
    web->addResponse(var["id"], "label", "Enabled");
  }, [this](JsonObject var, uint8_t rowNr) { //chFun

    if (rowNr != uint8Max) {
      modules[rowNr]->isEnabled = var["value"][rowNr];
      modules[rowNr]->enabledChanged();
    }
    else {
      USER_PRINTF(" no rowNr!!");
    }
    // print->printJson(" ", var);

  }, nullptr, modules.size(), [this](JsonObject var, uint8_t rowNr) { //valueFun
    var["value"][rowNr] = modules[rowNr]->isEnabled;
  });
}

void SysModules::loop() {
  // bool oneSec = false;
  bool tenSec = false;
  // if (millis() - oneSecondMillis >= 1000) {
  //   oneSecondMillis = millis();
  //   oneSec = true;
  // }
  // if (millis() - tenSecondMillis >= 10000) {
  //   tenSecondMillis = millis();
  //   tenSec = true;
  // }
  for (SysModule *module:modules) {
    if (module->isEnabled && module->success) {
      module->loop();
      if (millis() - module->oneSecondMillis >= 1000) {
        module->oneSecondMillis = millis();
        module->loop1s();
      }
      if (millis() - module->tenSecondMillis >= 10000) {
        module->tenSecondMillis = millis();
        module->loop10s();
      }
      // module->testManager();
      // module->performanceManager();
      // module->dataSizeManager();
      // module->codeSizeManager();
    }
  }
  if (newConnection) {
    newConnection = false;
    isConnected = true;
    connectedChanged();
  }

}

void SysModules::reboot() {
  for (SysModule *module:modules) {
    module->reboot();
  }
}

void SysModules::add(SysModule* module) {
  modules.push_back(module);
}

void SysModules::connectedChanged() {
  for (SysModule * module:modules) {
    module->connectedChanged();
  }
}