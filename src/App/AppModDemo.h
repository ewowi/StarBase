/*
   @title     StarBase
   @file      AppModDemo.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
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

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1100);

    JsonObject currentVar = ui->initCheckBox(parentVar, "on", true, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "On");
        return true;
      case onChange:
        //implement on/off behaviour
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    //logarithmic slider (10)
    currentVar = ui->initSlider(parentVar, "bri", 10, 0, 255, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Brightness");
        return true;
      case onChange: {
        return true; }
      default: return false; 
    }});
    currentVar["log"] = true; //logarithmic
    currentVar["dash"] = true; //these values override model.json???

    ui->initText(parentVar, "textField", "text");

    ui->initPin(parentVar, "blinkPin", blinkPin, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        ui->setLabel(var, "Blink Pin");
        ui->setComment(var, "🚧 tbd: reserved and allocated pins");
        return true; }
      case onChange: {
        //deallocate old value...
        pinsM->deallocatePin(var["oldValue"], "Blink");
        if (!var["value"].isNull()) {
          blinkPin = var["value"];
          pinsM->allocatePin(blinkPin, "Blink", "On board led");
          pinMode(blinkPin, OUTPUT); //tbd: part of allocatePin?
        }
        return true; }
      default: return false; 
    }});

    ui->initSlider(parentVar, "frequency", frequency, 0, UINT8_MAX, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        frequency = var["value"];
        return true;
      default: return false; 
    }});

  }

  void loop1s() {
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