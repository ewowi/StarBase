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
  ui->initCheckBox(tableVar, "mdlEnabled", false, false, [](JsonObject var) { //uiFun not readonly! (tbd)
    web->addResponse(var["id"], "label", "Enabled");
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