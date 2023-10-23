/*
   @title     StarMod
   @file      UserModE131.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include <ESPAsyncE131.h>

#include <vector>

#define maxChannels 20

struct VarToWatch {
  const char * id = nullptr;
  uint16_t max = -1;
  uint8_t savedValue = -1;
};

class UserModE131:public Module {

public:

  VarToWatch varsToWatch[maxChannels]; //up to 513

  UserModE131() :Module("e131-sACN") {
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    isEnabled = false; //default off

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);
  }

  // void connectedChanged() {
  //   setOnOff();
  // }

  // void enabledChanged() {
  //   setOnOff();
  // }

  void onOffChanged() {

    if (SysModModules::isConnected && isEnabled) {
      USER_PRINTF("UserModE131::connected && enabled\n");

      if (e131Created) { // TODO: crashes here - no idea why!
        USER_PRINTF("UserModE131 - ESPAsyncE131 created already\n");
        return;
      }
      USER_PRINTF("UserModE131 - Create ESPAsyncE131\n");

      e131 = ESPAsyncE131(universeCount);
      if (this->e131.begin(E131_MULTICAST, universe, universeCount)) { // TODO: multicast igmp failing, so only works with unicast currently
        USER_PRINTF("Network exists, begin e131.begin ok\n");
        success = true;
      }
      else {
        USER_PRINTF("Network exists, begin e131.begin FAILED\n");
      }
      e131Created = true;
    }
    else {
      e131Created = false;
    }
  }

  void loop() {
    // Module::loop();
    if(!e131Created) {
      return;
    }
    if (!e131.isEmpty()) {
      e131_packet_t packet;
      e131.pull(&packet);     // Pull packet from ring buffer

      for (int i=0; i < maxChannels; i++) {
        if (packet.property_values[i] != varsToWatch[i].savedValue) {

          USER_PRINTF("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH%d: %u -> %u",
                  htons(packet.universe),                 // The Universe for this packet
                  htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                  e131.stats.num_packets,                 // Packet counter
                  e131.stats.packet_errors,               // Packet error counter
                  i,
                  varsToWatch[i].savedValue,
                  packet.property_values[i]);             // Dimmer data for Channel i

          varsToWatch[i].savedValue = packet.property_values[i];

          if (varsToWatch[i].id != nullptr && varsToWatch[i].max != 0) {
            USER_PRINTF(" varsToWatch: %s\n", varsToWatch[i].id);
            mdl->setValueI(varsToWatch[i].id, varsToWatch[i].savedValue%(varsToWatch[i].max+1)); // TODO: ugly to have magic string 
          }
          else
            USER_PRINTF("\n");
        }
      }
    }
  }

  void patchChannel(uint8_t channel, const char * id, uint8_t max = 255) {
    varsToWatch[channel].id = id;
    varsToWatch[channel].savedValue = 0; // Always reset when (re)patching so variable gets set to DMX value even if unchanged
    varsToWatch[channel].max = max;
  }

  // uint8_t getValue(const char * id) {
  //   for (int i=0; i < maxChannels; i++) {
  //       if(varsToWatch[i].id == id) {
  //         return varsToWatch[i].savedValue;
  //       }
  //   }
  //   USER_PRINTF("ERROR: failed to find param %s\n", id);
  //   return 0;
  // }

  private:
    ESPAsyncE131 e131;
    boolean e131Created = false;
    uint16_t universe = 1;
    uint8_t universeCount = 1;

};

static UserModE131 *e131mod;