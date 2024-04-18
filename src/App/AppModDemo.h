/*
   @title     StarMod
   @file      AppModDemo.h
   @date      20240411
   @repo      https://github.com/ewowi/StarMod, submit changes to this file as PRs to ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

class AppModDemo: public SysModule {

public:

  unsigned long lastMillis;
  uint16_t blinkPin = UINT16_MAX;
  uint8_t frequency;

  AppModDemo() :SysModule("AppMod Demo") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1100);

    ui->initText(parentVar, "textField", "text");

    ui->initPin(parentVar, "blinkPin", blinkPin, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Blink Pin");
        ui->setComment(var, "🚧 tbd: reserved and allocated pins");
        return true; }
      case f_ChangeFun: {
        //deallocate old value...
        pins->deallocatePin(var["oldValue"], "Blink");
        if (!var["value"].isNull()) {
          blinkPin = var["value"];
          pins->allocatePin(blinkPin, "Blink", "On board led");
          pinMode(blinkPin, OUTPUT); //tbd: part of allocatePin?
        }
        return true; }
      default: return false; 
    }});

    ui->initCheckBox(parentVar, "on");

    ui->initSlider(parentVar, "frequency", frequency, 0, UINT8_MAX, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ChangeFun:
        frequency = var["value"];
        return true;
      default: return false; 
    }});

  }

  void loop1s() {
    // SysModule::loop();
    if (blinkPin != UINT16_MAX && millis() - lastMillis >= frequency) {
      lastMillis = millis();
      // ppf(" %d: %d", blinkPin,  digitalRead(blinkPin));
      int value = digitalRead(blinkPin);
      digitalWrite(blinkPin, value == LOW?HIGH:LOW);
    }

  }

  void onOffChanged() {
    if (mdls->isConnected && isEnabled) {
    } else {
    }
  }

};

extern AppModDemo *appModDemo;