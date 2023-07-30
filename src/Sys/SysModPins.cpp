/*
   @title     StarMod
   @file      SysModPins.cpp
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModPins.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"

SysModPins::SysModPins() :Module("Pins") {
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(33, OUTPUT);

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

  ui->initCheckBox(parentVar, "pin2", true, false, nullptr, updateGPIO);
  ui->initCheckBox(parentVar, "pin4", false, false);
  ui->initCheckBox(parentVar, "pin33", true, false);

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModPins::loop(){
  // Module::loop();
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

void SysModPins::registerPin(uint8_t pinNr) {
  
}