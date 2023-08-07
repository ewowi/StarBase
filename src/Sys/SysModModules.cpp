/*
   @title     StarMod
   @file      Modules.cpp
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "SysModModules.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"

bool Modules::newConnection = false;
std::vector<Module *> Modules::modules;

Modules::Modules() :Module("Modules") {
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

void Modules::setup() {
  for (Module *module:modules) {
    module->setup();
  }

  //do its own setup
  parentVar = ui->initModule(parentVar, name);

  JsonObject tableVar = ui->initTable(parentVar, "modules", nullptr, false, [](JsonObject var) { //uiFun
    // web->addResponse(var["id"], "label", "Files");
    web->addResponse(var["id"], "comment", "List of modules (enabled should be editable - wip)");
    JsonArray rows = web->addResponseA(var["id"], "table");
    for (Module *module:Modules::modules) {
      JsonArray row = rows.createNestedArray();
      row.add(module->name);  //create a copy!
      row.add(module->success);
      row.add(module->enabled);
    }
  });
  ui->initText(tableVar, "mdlName", nullptr, true, [](JsonObject var) { //uiFun
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
    //if value not array, create and initialize
    uint8_t rowNr = 0;
    if (!var["value"].is<JsonArray>()) {
      var.createNestedArray("value");
      for (Module *module: modules) {
        var["value"][rowNr] = module->enabled;
        rowNr++;
      }
    } else { //read array and set module enabled
      for (bool enabled:var["value"].as<JsonArray>()) {
        if (modules[rowNr]->enabled != enabled) {
          print->print("  mdlEnabled.chFun %d %s: %d->%d\n", rowNr, modules[rowNr]->name, modules[rowNr]->enabled, enabled);
          modules[rowNr]->enabled = enabled;
          modules[rowNr]->enabledChanged(enabled);
        }
        rowNr++;
      }
    }
  });
}

void Modules::loop() {
  for (Module *module:modules) {
    if (module->enabled && module->success) {
      module->loop();
      // module->testManager();
      // module->performanceManager();
      // module->dataSizeManager();
      // module->codeSizeManager();
    }
  }
  if (newConnection) {
    connected();
    newConnection = false;
  }
}

void Modules::add(Module* module) {
  modules.push_back(module);
}

void Modules::connected() {
  for (Module *module:modules) {
    module->connected();
  }
}