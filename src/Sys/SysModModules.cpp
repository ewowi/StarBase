/*
   @title     StarMod
   @file      Modules.cpp
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "SysModModules.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"

bool SysModModules::newConnection = false;
bool SysModModules::isConnected = false;

std::vector<Module *> SysModModules::modules;

SysModModules::SysModModules() :Module("Modules") {
  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

void SysModModules::setup() {
  for (Module *module:modules) {
    module->setup();
  }

  //do its own setup: will be shown as last module
  parentVar = ui->initModule(parentVar, name);

  JsonObject tableVar = ui->initTable(parentVar, "mdlTbl", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Modules");
    web->addResponse(var["id"], "comment", "List of modules");
    JsonArray rows = web->addResponseA(var["id"], "table");
    for (Module *module:SysModModules::modules) {
      JsonArray row = rows.createNestedArray();
      row.add(module->name);  //create a copy!
      row.add(module->success);
      row.add(module->isEnabled);
    }
  });
  ui->initText(tableVar, "mdlName", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
  });
  ui->initCheckBox(tableVar, "mdlSucces", false, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Success");
  });
  ui->initCheckBox(tableVar, "mdlEnabled", true, false, [](JsonObject var) { //uiFun not readonly! (tbd)
    //initially set to true, but as enabled are table cells, will be updated to an array
    web->addResponse(var["id"], "label", "Enabled");
  }, [](JsonObject var) { //chFun
    print->printJson("mdlEnabled.chFun", var);
    uint8_t rowNr = 0;

    //if value not array, create array
    if (!var["value"].is<JsonArray>()) //comment if forced to recreate enabled array
      var.createNestedArray("value");

    //if value array not same size as nr of modules
    if (var["value"].size() != modules.size()) {
      for (Module *module: modules) {
        var["value"][rowNr] = module->isEnabled;
        rowNr++;
      }
    } else { //read array and set module enabled
      for (bool isEnabled:var["value"].as<JsonArray>()) {
        if ((modules[rowNr]->isEnabled && !isEnabled) || (!modules[rowNr]->isEnabled && isEnabled)) {
          USER_PRINTF("  mdlEnabled.chFun %d %s: %d->%d\n", rowNr, modules[rowNr]->name, modules[rowNr]->isEnabled, isEnabled);
          modules[rowNr]->isEnabled = isEnabled;
          modules[rowNr]->enabledChanged();
        }
        rowNr++;
      }
    }
  });
}

void SysModModules::loop() {
  for (Module *module:modules) {
    if (module->isEnabled && module->success) {
      module->loop();
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
  if (millis() - secondMillis >= 5000) {
    secondMillis = millis();
    USER_PRINTF("❤️"); //heartbeat
  }

}

void SysModModules::add(Module* module) {
  modules.push_back(module);
}

void SysModModules::connectedChanged() {
  for (Module *module:modules) {
    module->connectedChanged();
  }
}