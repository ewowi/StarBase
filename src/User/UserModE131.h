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

#define maxChannels 513

class UserModE131:public SysModule {

public:

  UserModE131() :SysModule("e131-sACN") {
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    SysModule::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    parentVar = ui->initModule(parentVar, name);

    ui->initNumber(parentVar, "dun", universe, 0, 7, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "DMX Universe");
    }, [this](JsonObject var, uint8_t) { //chFun
      universe = var["value"];
    });

    JsonObject currentVar = ui->initNumber(parentVar, "dch", 1, 1, 512, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "DMX Channel");
      web->addResponse(var["id"], "comment", "First channel");
    }, [](JsonObject var, uint8_t) { //chFun
      ui->processUiFun("e131Tbl"); //rebuild table
    });
    currentVar["stage"] = true;

    JsonObject tableVar = ui->initTable(parentVar, "e131Tbl", nullptr, false, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Vars to watch");
      web->addResponse(var["id"], "comment", "List of instances");
      JsonArray rows = web->addResponseA(var["id"], "data");
      for (auto varToWatch: varsToWatch) {
        JsonArray row = rows.createNestedArray();
        row.add(varToWatch.channel + mdl->getValue("dch").as<uint8_t>());
        row.add((char *)varToWatch.id);
        row.add(varToWatch.max);
        row.add(varToWatch.savedValue);
      }
    });
    ui->initNumber(tableVar, "e131Channel", -1, 1, 512, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Channel");
    });
    ui->initText(tableVar, "e131Name", nullptr, 32, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Name");
    });
    ui->initNumber(tableVar, "e131Max", -1, 0, uint16Max, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Max");
    });
    ui->initNumber(tableVar, "e131Value", -1, 0, 255, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Value");
    });

  }

  // void connectedChanged() {
  //   setOnOff();
  // }

  // void enabledChanged() {
  //   setOnOff();
  // }

  void onOffChanged() {

    if (SysModules::isConnected && isEnabled) {
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
      // e131.end()???
      e131Created = false;
    }
  }

  void loop() {
    // SysModule::loop();
    if(!e131Created) {
      return;
    }
    if (!e131.isEmpty()) {
      e131_packet_t packet;
      e131.pull(&packet);     // Pull packet from ring buffer

      for (auto varToWatch=varsToWatch.begin(); varToWatch!=varsToWatch.end(); ++varToWatch) {
        for (int i=0; i < maxChannels; i++) {
          if (i == varToWatch->channel) {
            if (packet.property_values[i] != varToWatch->savedValue) {

              USER_PRINTF("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH%d: %u -> %u",
                      htons(packet.universe),                 // The Universe for this packet
                      htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                      e131.stats.num_packets,                 // Packet counter
                      e131.stats.packet_errors,               // Packet error counter
                      i,
                      varToWatch->savedValue,
                      packet.property_values[i]);             // Dimmer data for Channel i

              varToWatch->savedValue = packet.property_values[i];

              if (varToWatch->id != nullptr && varToWatch->max != 0) {
                USER_PRINTF(" varsToWatch: %s\n", varToWatch->id);
                mdl->setValueI(varToWatch->id, varToWatch->savedValue%(varToWatch->max+1)); // TODO: ugly to have magic string 
              }
              else
                USER_PRINTF("\n");
            }//!= savedValue
          }//if channel
        }//maxChannels
      } //for varToWatch
    } //!e131.isEmpty()
  } //loop

  void patchChannel(uint8_t channel, const char * id, uint8_t max = 255) {
    VarToWatch varToWatch;
    varToWatch.channel = channel;
    varToWatch.id = id;
    varToWatch.savedValue = 0; // Always reset when (re)patching so variable gets set to DMX value even if unchanged
    varToWatch.max = max;
    varsToWatch.push_back(varToWatch);
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
    struct VarToWatch {
      uint16_t channel;
      const char * id = nullptr;
      uint16_t max = -1;
      uint8_t savedValue = -1;
    };

    std::vector<VarToWatch> varsToWatch;

    ESPAsyncE131 e131;
    boolean e131Created = false;
    uint16_t universe = 1;
    uint8_t universeCount = 1;

};

static UserModE131 *e131mod;