/*
   @title     StarMod
   @file      UserModInstances.h
   @date      20231016
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

struct UDPWLEDMessage {
  byte token;       //0: 'binary token 255'
  byte id;          //1: id '1'
  byte ip0;         //2: uint32_t ip takes 6 bytes instead of 4 ?!?! so splitting it into bytes
  byte ip1;         //3
  byte ip2;         //4
  byte ip3;         //5
  char name[32];    //6..37: server name
  byte type;        //38: node type id
  byte nodeId;      //39: node id
  uint32_t version; //40..43: version ID (here it takes 4 bytes (as it should)
}; //total 44 bytes!

//compatible with WLED nodes as it only interprets first 44 bytes
struct UDPStarModMessage {
  UDPWLEDMessage header;
  char body[1416]; //1460 - 44, one udp frame should be 1460, not 1472 (then split by network in 1460 and 12)
};

class UserModInstances:public Module {

public:

  static std::vector<NodeInfo> nodes;

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
      for (NodeInfo node: nodes) {
        JsonArray row = rows.createNestedArray();
        row.add((char *)node.ip.toString().c_str());
        row.add((char *)node.type);
        row.add(node.timeStamp / 1000);
        row.add(node.details);
        char urlString[32] = "http://";
        strncat(urlString, node.ip.toString().c_str(), sizeof(urlString)-1);
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

    if (sizeof(UDPWLEDMessage) != 44) {
      USER_PRINTF("Program error: Size of UDP message is not 44: %d\n", sizeof(UDPWLEDMessage));
      // USER_PRINTF("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }
    if (sizeof(UDPStarModMessage) != 1460) { //size of UDP Packet
      USER_PRINTF("Program error: Size of UDP message is not 44: %d\n", sizeof(UDPStarModMessage));
      // USER_PRINTF("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }
    
    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void onOffChanged() {
    if (SysModModules::isConnected && isEnabled) {
      udp2Connected = instanceUDP.begin(instanceUDPPort);
    } else {
      nodes.clear();
      ui->processUiFun("insTbl");
      //udp off ??
    }
  }

  void loop() {
    // Module::loop();

    //everysecond better to save on cpu?
    if (millis() - secondMillis >= 1000) {
      USER_PRINTF(".");
      secondMillis = millis();

      handleNotifications();
    }

  }

  void handleNotifications()
  {
    if(!SysModModules::isConnected) return;

    instanceUDP.flush(); //tbd: test if needed
 
    int packetSize = instanceUDP.parsePacket();

    if (packetSize) {
      IPAddress remoteIp = instanceUDP.remoteIP();
      // TODO: actually look at the contents of the packet to fetch version, name etc

      USER_PRINTF("UDPStarModMessage %s %d check %d or %d\n", remoteIp.toString().c_str(), packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage));

      if (packetSize == sizeof(UDPWLEDMessage)) { //WLED instance
        UDPWLEDMessage udpMessage;
        uint8_t *udpIn = (uint8_t *)&udpMessage;
        instanceUDP.read(udpIn, packetSize);

        updateNode(udpMessage);
      }
      else if (packetSize == sizeof(UDPStarModMessage)) {
        // uint8_t udpIn[1472+1];
        UDPStarModMessage starModMessage;
        uint8_t *udpIn = (uint8_t *)&starModMessage;
        instanceUDP.read(udpIn, packetSize);

        updateNode(starModMessage.header, starModMessage.body);
      }
      else {
        USER_PRINTF("packetSize %d not equal to %d or %d\n", packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage));
      }

    } //packetSize
    else {
      if (millis() - second30Millis >= 10000) {
        second30Millis = millis();
        if(!SysModModules::isConnected) return;

        sendSysInfoUDP(); 
        
        //instance is not catching it's own udp message...

      }
    }

    //remove inactive nodes
    // size_t index = 0;
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
    starModMessage.header.token = 255;
    starModMessage.header.id = 1;
    // starModMessage.header.ip = ip[0]*256*256*256 + ip[1]*256*256 + ip[2]*256 + ip[3];
    starModMessage.header.ip0 = ip[0];
    starModMessage.header.ip1 = ip[1];
    starModMessage.header.ip2 = ip[2];
    starModMessage.header.ip3 = ip[3];
    strncpy(starModMessage.header.name, mdl->getValue("serverName").as<const char *>(), 32);
    starModMessage.header.type = 32; //esp32 tbd: CONFIG_IDF_TARGET_ESP32S3 etc
    starModMessage.header.nodeId = ip[3];
    starModMessage.header.version = 2309280; //tbd
    strncpy(starModMessage.body, "Effect x, Projection y, Leds begin..end", 1416-1);

    updateNode(starModMessage.header, starModMessage.body); //temp? to show new node in list

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

  void updateNode( UDPWLEDMessage udpMessage, char *body = nullptr) {
    USER_PRINTF("Instance: %d.%d.%d.%d n:%s b:%s\n", udpMessage.ip0, udpMessage.ip1, udpMessage.ip2, udpMessage.ip3, udpMessage.name, body );
    // if (body) {
    //   USER_PRINTF("body %s\n", body);
    // }

    bool found = false;
    //iterate vector pointers so we can update the nodes
    for (std::vector<NodeInfo>::iterator node=nodes.begin(); node!=nodes.end(); ++node) {
      if (node->ip == IPAddress(udpMessage.ip0, udpMessage.ip1, udpMessage.ip2, udpMessage.ip3)) {
        found = true;
        node->timeStamp = millis(); //update timestamp
        strncpy(node->details, udpMessage.name, sizeof(node->details)-1); //update name (in case changed)
        if (body) {
          strncat(node->details, " ", sizeof(node->details)-1);
          strncat(node->details, body, sizeof(node->details)-1);
        }
      }
    }

    if (!found) {
      // USER_PRINTF("new node: %s (%u) %s\n", remoteIp.toString().c_str(), packetSize, nodeName);
      NodeInfo newNode;
      newNode.ip = IPAddress(udpMessage.ip0, udpMessage.ip1, udpMessage.ip2, udpMessage.ip3);
      if (body == nullptr)
        strcpy(newNode.type, "WLED");
      else
        strcpy(newNode.type, "StarMod");
      newNode.timeStamp = millis();
      strncpy(newNode.details, udpMessage.name, sizeof(newNode.details)-1);
      nodes.push_back(newNode);
      ui->processUiFun("ddpInst"); //show the new instance in the dropdown  
      ui->processUiFun("artInst"); //show the new instance in the dropdown  
    }

    ui->processUiFun("insTbl");

  }

  private:
    WiFiUDP instanceUDP;
    uint16_t instanceUDPPort = 65506;
    bool udp2Connected = false;
    unsigned long second30Millis = 0;
};

static UserModInstances *instances;

std::vector<NodeInfo> UserModInstances::nodes;
