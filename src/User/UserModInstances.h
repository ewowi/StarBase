/*
   @title     StarMod
   @file      UserModInstances.h
   @date      20231016
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once

#include <vector>
#include "ArduinoJson.h"
#include "UserModE131.h"

struct AppData {
  uint8_t bri;
  uint8_t fx;
  uint8_t palette;
  uint8_t projection;
};

struct SysData {
  unsigned long upTime;
  uint8_t type;
  uint8_t syncGroups;
  uint16_t dmxChannel; 
};

struct NodeInfo {
  IPAddress ip;
  char name[32];
  uint32_t version;
  unsigned long timeStamp; //when was the package received, used to check on aging
  SysData sys;
  AppData app;
};

struct UDPWLEDMessage {
  byte token;       //0: 'binary token 255'
  byte id;          //1: id '1'
  byte ip0;         //2: uint32_t ip takes 6 bytes instead of 4 ?!?! so splitting it into bytes
  byte ip1;         //3
  byte ip2;         //4
  byte ip3;         //5
  char name[32];    //6..37: server name
  byte type;        //38: node type id //esp32 tbd: CONFIG_IDF_TARGET_ESP32S3 etc
  byte nodeId;      //39: node id  unit ID == last IP number
  uint32_t version; //40..43: version ID (here it takes 4 bytes (as it should)
}; //total 44 bytes!

//compatible with WLED nodes as it only interprets first 44 bytes
struct UDPStarModMessage {
  UDPWLEDMessage header; // 44 bytes
  SysData sys;
  AppData app;
  char body[1460 - sizeof(UDPWLEDMessage) - sizeof(SysData) - sizeof(AppData)];
};

//WLED syncmessage
struct UDPWLEDSyncMessage { //see notify( in WLED
  byte protocol; //0
  byte callMode; //1
  uint8_t bri; //2
  byte rCol0; //3
  byte gCol0; //4
  byte bCol0; //5
  byte nightlightActive; //6
  byte nightlightDelayMins; //7
  uint8_t mainsegMode; //8
  uint8_t mainsegSpeed; //9
  byte wCol0; //10
  byte version; //11
  uint8_t col1[4]; //12
  uint8_t mainsegIntensity; //16
  uint8_t transitionDelay[2]; //17
  uint8_t palette; //19
  uint8_t col2[4]; //20
  byte followUp; //24
  uint8_t timebase[4]; //25
  byte tokiSource; //29
  uint8_t tokiTime[4]; //30
  uint8_t tokiMs[2]; //34
  uint8_t syncGroups; //36
  char body[1193 - 37]; //41 +(32*36)+0 = 1193
};

class UserModInstances:public Module {

public:

  static std::vector<NodeInfo> nodes; //static because used in uiFun

  UserModInstances() :Module("Instances") {
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    parentVar = ui->initModule(parentVar, name);

    JsonObject tableVar = ui->initTable(parentVar, "insTbl", nullptr, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Instances");
      web->addResponse(var["id"], "comment", "List of instances");
      JsonArray rows = web->addResponseA(var["id"], "table");
      for (auto node: nodes) {
        JsonArray row = rows.createNestedArray();
        row.add((char *)node.name);
        row.add((char *)node.ip.toString().c_str());
        // row.add(node.timeStamp / 1000);
        char text[100];
        if (node.sys.type == 0) {
          row.add("WLED");
          print->fFormat(text, sizeof(text)-1, "ver:%d up:%d br:%d fx:%d pal:%d syn:%d", node.version, node.sys.upTime, node.app.bri, node.app.fx, node.app.palette, node.sys.syncGroups);
        }
        else {
          row.add("StarMod");
          print->fFormat(text, sizeof(text)-1, "ver:%d up:%d br:%d fx:%d pal:%d pro:%d syn:%d d:%d", node.version, node.sys.upTime, node.app.bri, node.app.fx, node.app.palette, node.app.projection, node.sys.syncGroups, node.sys.dmxChannel);
        }

        row.add(text);
        char urlString[32] = "http://";
        strncat(urlString, node.ip.toString().c_str(), sizeof(urlString)-1);
        row.add((char *)urlString);  //create a copy!
      }
    });
    ui->initText(tableVar, "insName", nullptr, 32, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Name");
    });
    ui->initText(tableVar, "insIp", nullptr, 16, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "IP");
    });
    ui->initText(tableVar, "insType", nullptr, 16, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Type");
    });
    // ui->initNumber(tableVar, "insTime", -1, 0, (unsigned long)-1, true, [](JsonObject var) { //uiFun
    //   web->addResponse(var["id"], "label", "Timestamp");
    // });
    ui->initText(tableVar, "insDetail", nullptr, 1024, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Detail");
    });
    ui->initURL(tableVar, "insLink", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Show");
    });

    ui->initNumber(parentVar, "syncGroups", 0, 0, 99, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "0=no sync");
    }, [](JsonObject var) { //chFun
      ui->valChangedForInstancesTemp = true;
    });

    if (sizeof(UDPWLEDMessage) != 44) {
      USER_PRINTF("Program error: Size of UDP message is not 44: %d\n", sizeof(UDPWLEDMessage));
      // USER_PRINTF("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }
    if (sizeof(UDPStarModMessage) != 1460) { //size of UDP Packet
      // one udp frame should be 1460, not 1472 (then split by network in 1460 and 12)
      USER_PRINTF("Program error: Size of UDP message is not 44: %d\n", sizeof(UDPStarModMessage));
      // USER_PRINTF("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }

    USER_PRINTF("UDPWLEDSyncMessage %d %d %d", sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage), sizeof(UDPWLEDSyncMessage));
    
    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void onOffChanged() {
    if (SysModModules::isConnected && isEnabled) {
      udpConnected = notifierUdp.begin(notifierUDPPort); //sync
      udp2Connected = instanceUDP.begin(instanceUDPPort); //nodes
    } else {
      nodes.clear();
      ui->processUiFun("insTbl");
      //udp off ??
    }
  }

  void loop() {
    // Module::loop();

    handleNotifications();

    //if value changed or every x seconds
    if (ui->valChangedForInstancesTemp ||  millis() - updateMillis >= 10000) {
      updateMillis = millis();

      if(!SysModModules::isConnected) return;

      ui->valChangedForInstancesTemp = false;

      sendSysInfoUDP(); 

    }
  }

  void handleNotifications()
  {
    if(!SysModModules::isConnected) return;

    // instanceUDP.flush(); //tbd: test if needed

    int packetSize;

    if (udpConnected) {
      int packetSize = notifierUdp.parsePacket();

      //handle sync from WLED
      if (packetSize > 0) {
        IPAddress remoteIp = notifierUdp.remoteIP();

        USER_PRINTF("handleNotifications sync %s %d\n", remoteIp.toString().c_str(), packetSize);

        UDPWLEDSyncMessage udpSyncMessage;
        uint8_t *udpIn = (uint8_t *)&udpSyncMessage;
        notifierUdp.read(udpIn, packetSize);

        for (int i=0; i<40; i++) {
          Serial.printf("%d: %d\n", i, udpIn[i]);
        }

        USER_PRINTF("   %d %d p:%d\n", udpSyncMessage.bri, udpSyncMessage.mainsegMode, packetSize);

        std::vector<NodeInfo>::iterator node = findNode( remoteIp);

        node->sys.upTime = (udpSyncMessage.timebase[0] * 256*256*256 + 256*256*udpSyncMessage.timebase[1] + 256*udpSyncMessage.timebase[2] + udpSyncMessage.timebase[3]) / 1000;
        node->sys.syncGroups = udpSyncMessage.syncGroups;
        
        uint8_t mySync = mdl->getValue("syncGroups");
        if (mySync != 0 && node->sys.syncGroups == mySync) {
          if (node->app.bri != udpSyncMessage.bri) mdl->setValueI("bri", udpSyncMessage.bri);
        }

        node->app.bri = udpSyncMessage.bri;
        node->app.fx = udpSyncMessage.mainsegMode;
        node->app.palette = udpSyncMessage.palette;

        // for (size_t x = 0; x < packetSize; x++) {
        //   char xx = (char)udpIn[x];
        //   Serial.print(xx);
        // }
        // Serial.println();

        ui->processUiFun("insTbl");

        return;
      }
    }
 
    //handle nodes update
    if (udp2Connected) {
      packetSize = instanceUDP.parsePacket();

      if (packetSize > 0) {
        IPAddress remoteIp = instanceUDP.remoteIP();
        // USER_PRINTF("handleNotifications nodes %s %d check %d or %d\n", remoteIp.toString().c_str(), packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage));

        if (packetSize == sizeof(UDPWLEDMessage)) { //WLED instance
          UDPStarModMessage starModMessage;
          uint8_t *udpIn = (uint8_t *)&starModMessage.header;
          instanceUDP.read(udpIn, packetSize);

          starModMessage.sys.type = 0; //WLED

          updateNode(starModMessage);
        }
        else if (packetSize == sizeof(UDPStarModMessage)) {
          UDPStarModMessage starModMessage;
          uint8_t *udpIn = (uint8_t *)&starModMessage;
          instanceUDP.read(udpIn, packetSize);
          starModMessage.sys.type = 1; //Starmod

          updateNode(starModMessage);
        }
        else {
          //flush the data
          uint8_t udpIn[1472+1];
          notifierUdp.read(udpIn, packetSize);

          USER_PRINTF("packetSize %d not equal to %d or %d\n", packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage));
        }

      } //packetSize
      else {
      }
    }

    //remove inactive nodes
    for (std::vector<NodeInfo>::iterator node=nodes.begin(); node!=nodes.end(); ) {
      if (millis() - node->timeStamp > 32000) { //assuming a ping each 30 seconds
        node = nodes.erase(node);
        ui->processUiFun("insTbl");
        ui->processUiFun("ddpInst");
        ui->processUiFun("artInst");
      }
      else
        ++node;
    }
  }

  void sendSysInfoUDP()
  {
    if (!udp2Connected) return;

    IPAddress ip = WiFi.localIP();
    if (!ip || ip == IPAddress(255,255,255,255)) ip = IPAddress(4,3,2,1);

    UDPStarModMessage starModMessage;
    starModMessage.header.token = 255; //WLED only accepts 255
    starModMessage.header.id = 1; //WLED only accepts 1
    starModMessage.header.ip0 = ip[0];
    starModMessage.header.ip1 = ip[1];
    starModMessage.header.ip2 = ip[2];
    starModMessage.header.ip3 = ip[3];
    const char * serverName = mdl->getValue("serverName");
    strncpy(starModMessage.header.name, serverName?serverName:"StarMod", sizeof(starModMessage.header.name)-1);
    starModMessage.header.type = 32; //esp32 tbd: CONFIG_IDF_TARGET_ESP32S3 etc
    starModMessage.header.nodeId = ip[3]; //WLED: used in map of nodes as index!
    starModMessage.header.version = atoi(SysModSystem::version);
    starModMessage.sys.type = 1; //StarMod
    starModMessage.sys.upTime = millis()/1000;
    starModMessage.sys.syncGroups = mdl->getValue("syncGroups");
    starModMessage.sys.dmxChannel = 0;
    #ifdef USERMOD_E131
      if (e131mod->isEnabled)
        starModMessage.sys.dmxChannel = mdl->getValue("dmxChannel");
    #endif
    starModMessage.app.bri = mdl->getValue("bri");
    starModMessage.app.fx = mdl->getValue("fx");
    starModMessage.app.palette = mdl->getValue("palette");
    starModMessage.app.projection = mdl->getValue("projection");

    updateNode(starModMessage); //temp? to show own node in list as instance is not catching it's own udp message...

    IPAddress broadcastIP(255, 255, 255, 255);
    if (0 != instanceUDP.beginPacket(broadcastIP, instanceUDPPort)) {  // WLEDMM beginPacket == 0 --> error
      USER_PRINTF("sendSysInfoUDP %s s:%d p:%d i:%d\n", (uint8_t*)&starModMessage, sizeof(UDPStarModMessage), instanceUDPPort, ip[3]);
      // for (size_t x = 0; x < sizeof(UDPWLEDMessage); x++) {
      //   char* xx = (char*)&udpMessage;
      //   Serial.printf("%d: %d - %c\n", x, xx[x], xx[x]);
      // }

      instanceUDP.write((uint8_t*)&starModMessage, sizeof(UDPStarModMessage));
      instanceUDP.endPacket();
    }
    else {
      USER_PRINTF("sendSysInfoUDP error\n");
    }
  }

  void updateNode( UDPStarModMessage udpStarMessage) {
    IPAddress ip = IPAddress(udpStarMessage.header.ip0, udpStarMessage.header.ip1, udpStarMessage.header.ip2, udpStarMessage.header.ip3);

    bool found = false;
    for (auto node : nodes) 
    {
      if (node.ip == ip)
        found = true;
    }

    // USER_PRINTF("updateNode Instance: %s n:%s b:%s %d\n", ip.toString().c_str(), udpStarMessage.header.name, udpStarMessage.body, found);

    if (!found) {
      NodeInfo node;
      node.ip = ip;
      if (udpStarMessage.sys.type == 0) {//WLED only
        node.sys.type = 0; //WLED
        //updated in udp sync message:
        node.sys.upTime = 0;
        node.sys.dmxChannel = 0;
        node.sys.syncGroups = 0;
        node.app.bri = 0; 
        node.app.fx = 0;
        node.app.palette = 0;
        node.app.projection = 0; //not used
      }

      nodes.push_back(node);
      std::sort(nodes.begin(),nodes.end(), [](NodeInfo &a, NodeInfo &b){ return strcmp(a.name,b.name); });//Sorting the vector
    }

    //iterate vector pointers so we can update the nodes
    for (std::vector<NodeInfo>::iterator node=nodes.begin(); node!=nodes.end(); ++node) {
      if (node->ip == ip) {
        node->timeStamp = millis(); //update timestamp
        strncpy(node->name, udpStarMessage.header.name, sizeof(node->name)-1);
        node->version = udpStarMessage.header.version;
        if (udpStarMessage.sys.type == 1) {//StarMod only
          node->sys = udpStarMessage.sys;
          if (found || (node->ip != ip && !found)) { //this is a new node, it should not update all the others
            uint8_t mySync = mdl->getValue("syncGroups");
            if (mySync != 0 && node->sys.syncGroups == mySync) {
              if (node->app.bri != udpStarMessage.app.bri) mdl->setValueI("bri", udpStarMessage.app.bri);
              if (node->app.fx != udpStarMessage.app.fx) mdl->setValueI("fx", udpStarMessage.app.fx);
              if (node->app.palette != udpStarMessage.app.palette) mdl->setValueI("palette", udpStarMessage.app.palette);
              if (node->app.projection != udpStarMessage.app.projection) mdl->setValueI("projection", udpStarMessage.app.projection);
            }
          }
          node->app = udpStarMessage.app;
        }
      }
    }
    if (!found) {
      ui->processUiFun("ddpInst"); //show the new instance in the dropdown  
      ui->processUiFun("artInst"); //show the new instance in the dropdown  
    }

    ui->processUiFun("insTbl");

  }

  std::vector<NodeInfo>::iterator findNode( IPAddress ip) {

    bool found = false;
    for (auto node : nodes) 
    {
      if (node.ip == ip)
        found = true;
    }
    if (!found) {
      NodeInfo node;
      node.ip = ip;
      nodes.push_back(node);
    }

    std::vector<NodeInfo>::iterator foundNode;
    //iterate vector pointers so we can update the nodes
    for (auto node=nodes.begin(); node!=nodes.end(); ++node) {
      if (node->ip == ip) {
        foundNode = node;
      }
    }

    return foundNode;

  }

  private:
    //sync
    WiFiUDP notifierUdp;
    uint16_t notifierUDPPort = 21324;
    bool udpConnected = false;

    //instances
    WiFiUDP instanceUDP;
    uint16_t instanceUDPPort = 65506;
    bool udp2Connected = false;

    unsigned long updateMillis = 0;
};

static UserModInstances *instances;

std::vector<NodeInfo> UserModInstances::nodes;