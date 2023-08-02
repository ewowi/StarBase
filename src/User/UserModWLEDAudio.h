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
      print->print("WLED-Sync: ");
      for (int b = 0; b < NUM_GEQ_CHANNELS; b++) {
        uint8_t val = sync.fftResult[b];
        print->print("%u ", val);
      }
      print->print("\n");
    }
  }

  private:
    WLEDSync sync; 

};

static UserModWLEDAudio *wledAudioMod;