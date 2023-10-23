/*
   @title     StarMod
   @file      SysModPins.cpp
   @date      20231016
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
  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(33, OUTPUT);

  //start with no pins allocated
  for (int i=0; i<NUM_PINS; i++) {
    deallocatePin(i, pinObjects[i].owner);
  }

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

void SysModPins::setup() {
  Module::setup();
  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = ui->initModule(parentVar, name);

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
  ui->initNumber(tableVar, "pinNr", 0, 0, NUM_PINS, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Pin");
  });
  ui->initText(tableVar, "pinOwner", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Owner");
  });
  ui->initText(tableVar, "pinDetails", nullptr, 256, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Details");
  });

  ui->initCanvas(parentVar, "board", -1, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Board layout");
    web->addResponse(var["id"], "comment", "WIP");
  }, nullptr, [](JsonObject var, uint8_t* buffer) { //loopFun
    // send leds preview to clients
    for (size_t i = 0; i < buffer[0] * 256 + buffer[1]; i++)
    {
      buffer[i*3+4] = random(256);// (digitalRead(i)+1) * 50;
      buffer[i*3+4+1] = random(256);;
      buffer[i*3+4+2] = random(256);;
    }
    //new values
    buffer[0] = 0; //0 * 256
    buffer[1] = 20; //20 pins
    buffer[3] = 10*10; //every 10 sec 
  });

  // ui->initCheckBox(parentVar, "pin2", true, false, nullptr, updateGPIO);
  // ui->initCheckBox(parentVar, "pin4");
  // ui->initCheckBox(parentVar, "pin33", true);

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
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
    bool pinValue = var["value"];
    JsonString id = var["id"];

    USER_PRINTF("updateGPIO %s:=%d\n", id.c_str(), pinValue);

    if (id == "pin2") digitalWrite(2, pinValue?HIGH:LOW);
    if (id == "pin4") digitalWrite(4, pinValue?HIGH:LOW);
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