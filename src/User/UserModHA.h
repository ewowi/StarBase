/*
   @title     StarMod
   @file      UserModHA.h
   @date      20240411
   @repo      https://github.com/ewowi/StarMod, submit changes to this file as PRs to ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include <ArduinoHA.h>

// Basic example of MQTT connectivity to Home Assistant.
// Add HALight, HASelect etc as required
class UserModHA:public SysModule {

public:

  UserModHA() :SysModule("Home Assistant support - template") {
    isEnabled = false;
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6300);

    ui->initText(parentVar, "mqttAddr");
  }


  void connectedChanged() {
    ppf("connectedChanged\n");
    if (mdls->isConnected) {
      // set device's details (optional)
      device.setName(_INIT(TOSTRING(APP)));
      device.setSoftwareVersion(_INIT(TOSTRING(VERSION)));
    }

    byte mac[] = {0xF1, 0x10, 0xFA, 0x6E, 0x38, 0x4A}; // TODO
    device.setUniqueId(mac, sizeof(mac));

    String mqttAddr = mdl->getValue("mqttAddr");

    ppf("mqtt->begin(%s)\n", mqttAddr.c_str());
    IPAddress ip;
    if(ip.fromString(mqttAddr)) {
      mqtt->begin(ip, "", "");
    }
    else {
      ppf("Failed to parse %s to IP\n", mqttAddr.c_str());
    }

  }

  void loop() {
    // SysModule::loop();
    mqtt->loop();
  }

  private:
    WiFiClient client;
    HADevice device;
    HAMqtt* mqtt = new HAMqtt(client, device);
};

extern UserModHA *hamod;