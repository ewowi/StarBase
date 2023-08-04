/*
   @title     StarMod
   @file      UserModExample.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include <WLED-sync.h> // https://github.com/netmindz/WLED-sync

class UserModWLEDAudio:public Module {

public:

  uint8_t fftResults[NUM_GEQ_CHANNELS]= {0};

  UserModWLEDAudio() :Module("WLED Audio Sync Receiver") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void connected() {
    sync.begin();
  }

  void loop(){
    // Module::loop();
    if (sync.read()) {
      if(debug) print->print("WLED-Sync: ");
      for (int b = 0; b < NUM_GEQ_CHANNELS; b++) {
        uint8_t val = sync.fftResult[b];
        fftResults[b] = val;
        if(debug) print->print("%u ", val);
      }
      if(debug) print->print("\n");
    }
  }

  private:
    WLEDSync sync;
    boolean debug; 

};

static UserModWLEDAudio *wledAudioMod;