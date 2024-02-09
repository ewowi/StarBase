/*
   @title     StarMod
   @file      UserModHA.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include <ArduinoHA.h>

#define BROKER_ADDR     IPAddress(192,168,178,42) //ewowi: could we scan that instead of hard coded?

class UserModHA:public SysModule {

public:

  UserModHA() :SysModule("Home Assistant support") {
    isEnabled = false;
  };

  static void onStateCommand(bool state, HALight* sender) {
      Serial.print("State: ");
      Serial.println(state);

      sender->setState(state); // report state back to the Home Assistant
  }

  static void onBrightnessCommand(uint8_t brightness, HALight* sender) {
      Serial.print("Brightness: ");
      Serial.println(brightness);

      sender->setBrightness(brightness); // report brightness back to the Home Assistant
  }

  static void onRGBColorCommand(HALight::RGBColor color, HALight* sender) {
      Serial.print("Red: ");
      Serial.println(color.red);
      Serial.print("Green: ");
      Serial.println(color.green);
      Serial.print("Blue: ");
      Serial.println(color.blue);

      sender->setRGBColor(color); // report color back to the Home Assistant
  }

  void connectedChanged() {
    if (SysModules::isConnected) {
      // set device's details (optional)
      device.setName("StarMod");
      device.setSoftwareVersion("0.0.1");
    }

    // configure light (optional)
    light->setName("LEDs");

    // Optionally you can set retain flag for the HA commands
    // light.setRetain(true);

    // Maximum brightness level can be changed as follows:
    // light.setBrightnessScale(50);

    // Optionally you can enable optimistic mode for the HALight.
    // In this mode you won't need to report state back to the HA when commands are executed.
    // light.setOptimistic(true);

    // handle light states
    light->onStateCommand(onStateCommand);
    light->onBrightnessCommand(onBrightnessCommand); // optional
    light->onRGBColorCommand(onRGBColorCommand); // optional

    mqtt->begin(BROKER_ADDR);
  }

  void loop() {
    // SysModule::loop();
    mqtt->loop();
  }

  private:
    WiFiClient client;
    HADevice device;
    HAMqtt* mqtt = new HAMqtt(client, device);
    HALight* light = new HALight("starmod", HALight::BrightnessFeature | HALight::RGBFeature);
};

static UserModHA *hamod;