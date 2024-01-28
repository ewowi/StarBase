/*
   @title     StarMod
   @file      UserModWLEDAudio.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include <WLED-sync.h> // https://github.com/netmindz/WLED-sync

#define MAX_FREQUENCY   11025          // sample frequency / 2 (as per Nyquist criterion)

class UserModWLEDAudio:public SysModule {

public:

  WLEDSync sync;
  uint8_t fftResults[NUM_GEQ_CHANNELS]= {0};

  UserModWLEDAudio() :SysModule("WLED Audio Sync Receiver") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();
  }

  void onOffChanged() {
    if (SysModules::isConnected && isEnabled) {
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
    boolean debug = false; 

};

static UserModWLEDAudio *wledAudioMod;