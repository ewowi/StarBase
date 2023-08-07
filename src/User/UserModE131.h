/*
   @title     StarMod
   @file      UserModE131.h
   @date      20230807
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include <ESPAsyncE131.h>

#include <vector>

class UserModE131:public Module {

public:

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
          if (packet.property_values[i] != mdl->varsToWatch[i].savedValue) {

            print->print("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH%d: %u -> %u",
                    htons(packet.universe),                 // The Universe for this packet
                    htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                    e131.stats.num_packets,                 // Packet counter
                    e131.stats.packet_errors,               // Packet error counter
                    i,
                    mdl->varsToWatch[i].savedValue,
                    packet.property_values[i]);             // Dimmer data for Channel i

            mdl->varsToWatch[i].savedValue = packet.property_values[i];

            if (mdl->varsToWatch[i].id != nullptr) {
              print->print(" var: %s\n", mdl->varsToWatch[i].id);
              mdl->setValueI(mdl->varsToWatch[i].id, mdl->varsToWatch[i].savedValue%mdl->varsToWatch[i].max); // TODO: ugly to have magic string 
            }
            else
              print->print("\n");
          }
        }
    }
  }

  private:
    ESPAsyncE131 e131;
    boolean e131Created = false;
    int universe, universeCount = 1;

};

static UserModE131 *e131mod;