/*
   @title     StarMod
   @file      UserModInstances.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include <vector>
#include "ArduinoJson.h"

struct NodeInfo {
  IPAddress ip;
  char type[8];
  unsigned long timeStamp;
  char details[64];
  JsonVariant json; //tbd
};

class UserModInstances:public Module {

public:

  static std::vector<NodeInfo> nodes;

  UserModInstances() :Module("Instances") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentVar = ui->initModule(parentVar, name);

    JsonObject tableVar = ui->initTable(parentVar, "insTbl", nullptr, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Instances");
      web->addResponse(var["id"], "comment", "List of instances");
      JsonArray rows = web->addResponseA(var["id"], "table");
      for (NodeInfo node: nodes) {
        JsonArray row = rows.createNestedArray();
        row.add((char *)node.ip.toString().c_str());
        row.add((char *)node.type);
        row.add(node.timeStamp / 1000);
        row.add(node.details);
        char urlString[32] = "http://";
        strcat(urlString, node.ip.toString().c_str());
        row.add((char *)urlString);  //create a copy!
      }
    });
    ui->initText(tableVar, "insIp", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "IP");
    });
    ui->initText(tableVar, "insType", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Type");
    });
    ui->initText(tableVar, "insTime", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Timestamp");
    });
    ui->initText(tableVar, "insDetail", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Detail");
    });
    ui->initURL(tableVar, "insLink", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Show");
    });

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void onOffChanged() {
    if (SysModModules::isConnected && isEnabled) {
      udp.begin(65506);
    } else {
      nodes.clear();
      ui->processUiFun("insTbl");
      //udp off ??
    }
  }

  void loop(){
    // Module::loop();

    //everysecond better to save on cpu?
    if (millis() - secondMillis >= 1000) {
      secondMillis = millis();
      if(!SysModModules::isConnected) return;

      int packetSize = udp.parsePacket();

      if (packetSize) {
        IPAddress remoteIp = udp.remoteIP();
        // TODO: actually look at the contents of the packet to fetch version, name etc
        uint8_t udpIn[1472+1];
        udp.read(udpIn, packetSize);
        //for WLED see handleNotifications and sendSysInfoUDP
        //  0: 1 byte 'binary token 255'
        //  1: 1 byte id '1'
        //  2: 4 byte ip
        //  6: 32 char name
        // 38: 1 byte node type id
        // 39: 1 byte node id
        // 40: 4 byte version ID
        // 44 bytes total
        char nodeName[33] = { 0 };
        memcpy(&nodeName[0], reinterpret_cast<byte *>(&udpIn[6]), 32);
        nodeName[32] = 0;

        udp.read(packetBuffer, packetSize);

        print->print("Instance: %s (%u) %s\n", remoteIp.toString().c_str(), packetSize, nodeName);

        bool found = false;
        //iterate vector pointers so we can update the nodes
        for (std::vector<NodeInfo>::iterator node=nodes.begin(); node!=nodes.end(); ++node) {
          if (node->ip == remoteIp) {
            found = true;
            node->timeStamp = millis(); //update timestamp
            // strcpy(node->details, nodeName); //update name (in case changed)
          }
        }

        if (!found) {
          // print->print("new node: %s (%u) %s\n", remoteIp.toString().c_str(), packetSize, nodeName);
          NodeInfo newNode;
          newNode.ip = remoteIp;
          if (packetSize == 44)
            strcpy(newNode.type, "WLED");
          else
            strcpy(newNode.type, "Unknown");
          newNode.timeStamp = millis();
          strcpy(newNode.details, nodeName);
          nodes.push_back(newNode);
          ui->processUiFun("ddpInst"); //show the new instance in the dropdown  
          ui->processUiFun("artInst"); //show the new instance in the dropdown  
        }
        ui->processUiFun("insTbl");
      } //packetSize

      //remove inactive nodes
      size_t index = 0;
      for (std::vector<NodeInfo>::iterator node=nodes.begin(); node!=nodes.end(); ++node) {
        if (millis() - node->timeStamp > 32000) { //assuming a ping each 30 seconds
          nodes.erase(nodes.begin() + index);
          ui->processUiFun("insTbl");
          ui->processUiFun("ddpInst");
          ui->processUiFun("artInst");
        }
        index++;
      }
    }
  }

  private:
    char packetBuffer[255];
    WiFiUDP udp;

};

static UserModInstances *instances;

std::vector<NodeInfo> UserModInstances::nodes;
