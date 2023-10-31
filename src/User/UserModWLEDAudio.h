/*
   @title     StarMod
   @file      UserModWLEDAudio.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once

#include <WLED-sync.h> // https://github.com/netmindz/WLED-sync

class UserModWLEDAudio:public SysModule {

public:

  uint8_t fftResults[NUM_GEQ_CHANNELS]= {0};

  UserModWLEDAudio() :SysModule("WLED Audio Sync Receiver") {
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    SysModule::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void onOffChanged() {
    if (SysModules::isConnected && isEnabled) {
      USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);
      sync.begin();
    } else {
      // sync.end();???
    }
  }

  void loop() {
    // SysModule::loop();
    if (SysModules::isConnected && sync.read()) {
      if(debug) USER_PRINTF("WLED-Sync: ");
      for (int b = 0; b < NUM_GEQ_CHANNELS; b++) {
        uint8_t val = sync.fftResult[b];
        fftResults[b] = val;
        if(debug) USER_PRINTF("%u ", val);
      }
      if(debug) USER_PRINTF("\n");
    }
  }

  private:
    WLEDSync sync;
    boolean debug = false; 

};

static UserModWLEDAudio *wledAudioMod;