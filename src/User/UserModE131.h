/*
   @title     StarMod
   @file      UserModE131.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include <ESPAsyncE131.h>

#include <vector>

#define maxChannels 10

struct VarToWatch {
  const char * id = nullptr;
  uint16_t max = -1;
  uint8_t savedValue = -1;
};

class UserModE131:public Module {

public:

  VarToWatch varsToWatch[maxChannels]; //up to 513

  UserModE131() :Module("e131/sACN support") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);
  }

  void connected() {
    print->print("UserModE131::connected\n");
    if(e131Created) { // TODO: crashes here - no idea why!
      print->print("UserModE131 - ESPAsyncE131 created already\n");
      return;
    }
    print->print("UserModE131 - Create ESPAsyncE131\n");

    e131 = ESPAsyncE131(universeCount);
    if (this->e131.begin(E131_MULTICAST, universe, universeCount)) { // TODO: multicast igmp failing, so only works with unicast currently
      print->print("Network exists, begin e131.begin ok\n");
      success = true;
    }
    else {
      print->print("Network exists, begin e131.begin FALED\n");
    }
    e131Created = true;
    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    if(!e131Created) {
      return;
    }
    if (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet);     // Pull packet from ring buffer

        for (int i=0; i < maxChannels; i++) {
          if (packet.property_values[i] != varsToWatch[i].savedValue) {

            print->print("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH%d: %u -> %u",
                    htons(packet.universe),                 // The Universe for this packet
                    htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                    e131.stats.num_packets,                 // Packet counter
                    e131.stats.packet_errors,               // Packet error counter
                    i,
                    varsToWatch[i].savedValue,
                    packet.property_values[i]);             // Dimmer data for Channel i

            varsToWatch[i].savedValue = packet.property_values[i];

            if (varsToWatch[i].id != nullptr) {
              print->print(" var: %s\n", varsToWatch[i].id);
              mdl->setValueI(varsToWatch[i].id, varsToWatch[i].savedValue%varsToWatch[i].max); // TODO: ugly to have magic string 
            }
            else
              print->print("\n");
          }
        }
    }
  }

  void addWatch(uint8_t channel, const char * id, uint16_t max) {
    varsToWatch[channel].id = id;
    varsToWatch[channel].max = max;
  }

  private:
    ESPAsyncE131 e131;
    boolean e131Created = false;
    int universe, universeCount = 1;

};

static UserModE131 *e131mod;