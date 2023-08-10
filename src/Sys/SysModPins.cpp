/*
   @title     StarMod
   @file      SysModPins.cpp
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModPins.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"

PinObject SysModPins::pinObjects[NUM_PINS];
bool SysModPins::pinsChanged = false;

SysModPins::SysModPins() :Module("Pins") {
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(33, OUTPUT);

  //start with no pins allocated
  for (int i=0; i<NUM_PINS; i++) {
    deallocatePin(i, pinObjects[i].owner);
  }

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

void SysModPins::setup() {
  Module::setup();
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = ui->initModule(parentVar, name);

  ui->initCanvas(parentVar, "board", -1, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Board layout");
    web->addResponse(var["id"], "comment", "WIP");
  }, nullptr, [](JsonObject var, uint8_t* buffer) { //loopFun
    // send leds preview to clients
    for (size_t i = 0; i < buffer[0] * 256 + buffer[1]; i++)
    {
      buffer[i*3+4] = (digitalRead(i)+1) * 50;
      buffer[i*3+4+1] = 255;
      buffer[i*3+4+2] = 192;
    }
    //new values
    buffer[0] = 0; //0 * 256
    buffer[1] = 20; //20 pins
    buffer[3] = 10*10; //every 10 sec 
  });

  //show table of allocated pins
  JsonObject tableVar = ui->initTable(parentVar, "pinTbl", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Pins");
    web->addResponse(var["id"], "comment", "List of pins");
    JsonArray rows = web->addResponseA(var["id"], "table");
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
  ui->initNumber(tableVar, "pinNr", -1, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Pin");
  });
  ui->initText(tableVar, "pinOwner", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Owner");
  });
  ui->initText(tableVar, "pinDetails", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Details");
  });

  ui->initCheckBox(parentVar, "pin2", true, false, nullptr, updateGPIO);
  ui->initCheckBox(parentVar, "pin4", false, false);
  ui->initCheckBox(parentVar, "pin33", true, false);

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModPins::loop(){
  // Module::loop();

  if (millis() - secondMillis >= 1000) {
    secondMillis = millis();

    if (pinsChanged) {
      pinsChanged = false;

      ui->processUiFun("pinTbl");
    }
  }
}

void SysModPins::updateGPIO(JsonObject var) {
  if (var["value"].is<bool>()) {
    bool pin = var["value"];
    JsonString id = var["id"];

    print->print("updateGPIO %s:=%d\n", id.c_str(), pin);

    if (id == "pin2") digitalWrite(2, pin?HIGH:LOW);
    if (id == "pin4") digitalWrite(4, pin?HIGH:LOW);
    if (id == "pin33") digitalWrite(33, pin?HIGH:LOW);
  }
}

void SysModPins::allocatePin(uint8_t pinNr, const char * owner, const char * details) {
  print->print("allocatePin %d %s %s\n", pinNr, owner, details);
  if (pinNr < NUM_PINS) {
    if (strcmp(pinObjects[pinNr].owner, "") != 0 && strcmp(pinObjects[pinNr].owner, owner) != 0)
      print->print("allocatePin %d: not owner %s!=%s", pinNr, owner, pinObjects[pinNr].owner);
    else {
      strcpy(pinObjects[pinNr].owner, owner);  
      strcpy(pinObjects[pinNr].details, details);  
      pinsChanged = true;
    }
  }
}

void SysModPins::deallocatePin(uint8_t pinNr, const char * owner) {
  // print->print("deallocatePin %d %s\n", pinNr, owner);
  if (pinNr < NUM_PINS) {
    if (strcmp(pinObjects[pinNr].owner, owner) != 0)
      print->print("deallocatePin %d: not owner %s!=%s", pinNr, owner, pinObjects[pinNr].owner);
    else {
      strcpy(pinObjects[pinNr].owner, "");  
      strcpy(pinObjects[pinNr].details, "");  
      pinsChanged = true;
    }
  }
}