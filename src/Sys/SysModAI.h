/*
   @title     StarMod
   @file      SysModAI.h
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once


class SysModAI:public SysModule {

struct Recommendation {
  char intel[32] = "";
  char module[32] = "";
};

public:

  std::vector<Recommendation> recommendations;

  SysModAI() :SysModule("AI") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 5100);

    JsonObject tableVar = ui->initTable(parentVar, "aiTbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "AI recommendations");
        // ui->setComment(var, "List of instances");
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "aiIntel", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (unsigned8 rowNr = 0; rowNr < recommendations.size(); rowNr++)
          mdl->setValue(var, JsonString(recommendations[rowNr].intel, JsonString::Copied), rowNr);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Intel");
        return true;
      default: return false;
    }});

    ui->initButton(tableVar, "aiButton", false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (unsigned8 rowNr = 0; rowNr < recommendations.size(); rowNr++)
          mdl->setValue(var, JsonString(recommendations[rowNr].module, JsonString::Copied), rowNr);
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

    addIntelligence("Hello","World");
    addIntelligence("Create fixture","Fixture Generator");
  }

  void loop() {
    // SysModule::loop();
  } //loop

  void addIntelligence(const char *intel, const char *module) {
    Recommendation rec;
    strncpy(rec.intel, intel, 32-1);
    strncpy(rec.module, module, 32 - 1);
    USER_PRINTF("addIntelligence %s %s\n", intel, module);
    recommendations.push_back(rec);
  }

  void RemoveIntel(const char *recommendation) {
    
  }

};

static SysModAI *ai;

//  std::vector<Recommendation> SysModAI::recommendations;
