/*
   @title     StarMod
   @file      UserModExample.h
   @date      20240411
   @repo      https://github.com/ewowi/StarMod, submit changes to this file as PRs to ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

class UserModExample: public SysModule {

public:

  UserModExample() :SysModule("Usermod example") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();
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

extern UserModExample *example;