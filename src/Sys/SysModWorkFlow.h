/*
   @title     StarMod
   @file      SysModWorkFlow.h
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include "SysModModel.h"
#include "SysModUI.h"

struct Action {
  char description[32];
  char module[32];
};

class SysModWorkFlow: public SysModule {

public:

  bool rebuildTable = false;
  std::vector<Action> actions;

  SysModWorkFlow() :SysModule("Workflow") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1302); //appMod is it is shown on the app tab

    JsonObject tableVar = ui->initTable(parentVar, "wfTbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Workflow");
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "wfAction", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNr = 0; rowNr < actions.size(); rowNr++)
          mdl->setValue(var, JsonString(actions[rowNr].description, JsonString::Copied), rowNr);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Action");
        return true;
      default: return false;
    }});

    ui->initButton(tableVar, "wfButton", true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNr = 0; rowNr < actions.size(); rowNr++)
          mdl->setValue(var, JsonString(actions[rowNr].module, JsonString::Copied), rowNr);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Module");
        return true;
      // case f_ChangeFun:
      //   generateChFun(var);
      //   //reload fixture select
      //   ui->callVarFun("fixture", UINT8_MAX, f_UIFun);
      //   return true;
      default: return false;
    }});

    // addAction("Hello","World");
    // addAction("Create fixture","Fixture Generator");
  }

  void loop() {
    // SysModule::loop();
    if (rebuildTable) {
      rebuildTable = false;
      for (JsonObject childVar: mdl->varChildren("wfTbl"))
        ui->callVarFun(childVar, UINT8_MAX, f_ValueFun);
    }
  } //loop

  void addAction(const char *description, const char *module) {
    Action action;
    strcpy(action.description, description);
    strcpy(action.module, module);
    USER_PRINTF("addAction %s %s %d\n", description, module, actions.size());
    actions.push_back(action);

    rebuildTable = true;
  }

  void RemoveAction(const char *description) {
    
  }

};

extern SysModWorkFlow *wfl;
