/*
   @title     StarMod
   @file      SysModPins.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModPins.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"

PinObject SysModPins::pinObjects[NUM_PINS];
bool SysModPins::pinsChanged = false;

SysModPins::SysModPins() :SysModule("Pins") {
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(33, OUTPUT);

  //start with no pins allocated
  for (int i=0; i<NUM_PINS; i++) {
    deallocatePin(i, pinObjects[i].owner);
  }
};

void SysModPins::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name);
  if (parentVar["o"] > -1000) parentVar["o"] = -2200; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

  //show table of allocated pins
  JsonObject tableVar = ui->initTable(parentVar, "pinTbl", nullptr, true, [](JsonObject var) { //uiFun ro true: no update and delete
    web->addResponse(var["id"], "label", "Pins");
    web->addResponse(var["id"], "comment", "List of pins");
    JsonArray rows = web->addResponseA(var["id"], "value"); //overwrite the value
    uint8_t pinNr = 0;
    for (PinObject pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        JsonArray row = rows.createNestedArray();
        row.add(pinNr);
        row.add(pinObject.owner);
        row.add(pinObject.details);
      }
      pinNr++;
    }
  });
  ui->initNumber(tableVar, "pinNr", UINT16_MAX, 0, NUM_PINS, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Pin");
  });
  ui->initText(tableVar, "pinOwner", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Owner");
  });
  ui->initText(tableVar, "pinDetails", nullptr, 256, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Details");
  });

  ui->initCanvas(parentVar, "board", UINT16_MAX, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Board layout");
    web->addResponse(var["id"], "comment", "WIP");
  }, nullptr, [](JsonObject var, uint8_t rowNr) { //loopFun

    var["interval"] = 10*10*10; //every 10 sec from cs to ms

    web->sendDataWs([](AsyncWebSocketMessageBuffer * wsBuf) {
      uint8_t* buffer;

      buffer = wsBuf->get();

      // send leds preview to clients
      for (size_t i = 0; i < 20; i++)
      {
        buffer[i*3+5] = random(256);// (digitalRead(i)+1) * 50;
        buffer[i*3+5+1] = random(256);
        buffer[i*3+5+2] = random(256);
      }
      //new values
      buffer[0] = 0; //userFun id

    }, 20 * 3 + 5, true);
  });

  // ui->initCheckBox(parentVar, "pin2", true, false, nullptr, updateGPIO);
  // ui->initCheckBox(parentVar, "pin4");
  ui->initCheckBox(parentVar, "pin19", true, false, nullptr, updateGPIO);
  // ui->initCheckBox(parentVar, "pin33", true);
}

void SysModPins::loop1s() {
  if (pinsChanged) {
    pinsChanged = false;

    ui->processUiFun("pinTbl");
  }
}

void SysModPins::updateGPIO(JsonObject var, size_t index) {
  if (var["value"].is<bool>()) {
    bool pinValue = var["value"];
    JsonString id = var["id"];

    USER_PRINTF("updateGPIO %s:=%d\n", id.c_str(), pinValue);

    if (id == "pin2") digitalWrite(2, pinValue?HIGH:LOW);
    if (id == "pin4") digitalWrite(4, pinValue?HIGH:LOW);
    if (id == "pin19") digitalWrite(19, pinValue?HIGH:LOW);
    if (id == "pin33") digitalWrite(33, pinValue?HIGH:LOW);
  }
}

void SysModPins::allocatePin(uint8_t pinNr, const char * owner, const char * details) {
  USER_PRINTF("allocatePin %d %s %s\n", pinNr, owner, details);
  if (pinNr < NUM_PINS) {
    if (strcmp(pinObjects[pinNr].owner, "") != 0 && strcmp(pinObjects[pinNr].owner, owner) != 0)
      USER_PRINTF("allocatePin %d: not owner %s!=%s", pinNr, owner, pinObjects[pinNr].owner);
    else {
      strncpy(pinObjects[pinNr].owner, owner, sizeof(PinObject::owner)-1);  
      strncpy(pinObjects[pinNr].details, details, sizeof(PinObject::details)-1);  
      pinsChanged = true;
    }
  }
}

void SysModPins::deallocatePin(uint8_t pinNr, const char * owner) {
  // USER_PRINTF("deallocatePin %d %s\n", pinNr, owner);
  if (pinNr < NUM_PINS) {
    if (strcmp(pinObjects[pinNr].owner, owner) != 0)
      USER_PRINTF("deallocatePin %d: not owner %s!=%s", pinNr, owner, pinObjects[pinNr].owner);
    else {
      strcpy(pinObjects[pinNr].owner, "");  
      strcpy(pinObjects[pinNr].details, "");  
      pinsChanged = true;
    }
  }
}