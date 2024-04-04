/*
   @title     StarMod
   @file      AppModDemo.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

class AppModDemo: public SysModule {

public:

  AppModDemo() :SysModule("AppMod Demo") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1100);

    ui->initText(parentVar, "textField");

  }

  void loop() {
    // SysModule::loop();
  }

  void onOffChanged() {
    if (mdls->isConnected && isEnabled) {
    } else {
    }
  }

};

extern AppModDemo *appModDemo;