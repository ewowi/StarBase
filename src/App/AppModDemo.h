/*
   @title     StarBase
   @file      AppModDemo.h
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

class AppModDemo: public SysModule {

public:

  unsigned long lastMillis;
  uint8_t blinkPin = UINT8_MAX;
  uint8_t frequency;

  AppModDemo() :SysModule("AppMod Demo") {
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1100);

    JsonObject currentVar = ui->initCheckBox(parentVar, "on", true, false, [](EventArguments) { switch (eventType) { //varFun
      case onChange:
        //implement on/off behaviour
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    //logarithmic slider (10)
    currentVar = ui->initSlider(parentVar, "brightness", 10, 0, 255, false, [](EventArguments) { switch (eventType) { //varFun
      case onChange: {
        return true; }
      default: return false; 
    }});
    currentVar["log"] = true; //logarithmic
    currentVar["dash"] = true; //these values override model.json???

    ui->initText(parentVar, "textField", "text");

    ui->initPin(parentVar, "blinkPin", &blinkPin, false, [this](EventArguments) { switch (eventType) { //varFun
      case onUI:
        variable.setComment("🚧 tbd: reserved and allocated pins");
        return true;
      case onChange: {
        //deallocate old value...
        uint8_t oldValue = variable.var["oldValue"];
        ppf("blinkPin onChange %d %d\n", oldValue, blinkPin);
        if (oldValue != UINT8_MAX)
          pinsM->deallocatePin(oldValue, "Blink");
        if (blinkPin != UINT8_MAX) {
          pinsM->allocatePin(blinkPin, "Blink", "On board led");
          pinMode(blinkPin, OUTPUT); //tbd: part of allocatePin?
        }
        return true; }
      default: return false; 
    }});

    ui->initSlider(parentVar, "frequency", &frequency);

  }

  void loop1s() {
    if (blinkPin != UINT8_MAX && millis() - lastMillis >= frequency) {
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