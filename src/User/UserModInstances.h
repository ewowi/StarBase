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
#ifdef USERMOD_E131
  #include "UserModE131.h"
#endif
// #include "Sys/SysModSystem.h" //for sys->version
#include <HTTPClient.h> //need to be replaced by udp messages as this is a memory sucker

struct DMX {
  uint8_t universe:3; //3 bits / 8
  uint16_t start:9; //9 bits / 512
  uint8_t count:4; // 4 bits / 16
}; //total 16 bits

struct SysData {
  unsigned long upTime;
  uint8_t type;
  uint8_t syncMaster;
  DMX dmx;
};

struct VarData {
  char id[3];
  uint8_t value;
}; //4

#define nrOfAppVars 20

struct AppData {
  VarData vars[nrOfAppVars]; //total 80

  int getVar(const char * varID) { //int to support -1
    for (int i=0; i< nrOfAppVars; i++) {
      if (strncmp(vars[i].id, "", 3) != 0 && strncmp(vars[i].id, varID, 3) == 0) {
        return vars[i].value;
      }
    }
    return -1;
  }
  
  void setVar(const char * varID, uint8_t value) {
    size_t foundAppVar;
    for (int i=0; i< nrOfAppVars; i++) {
      if (strncmp(vars[i].id, "", 3) == 0)
        foundAppVar = i;
      else if (strncmp(vars[i].id, varID, 3) == 0) {
        foundAppVar = i;
        break; //use this slot
      }
    }
    size_t i = foundAppVar;
    if (strncmp(vars[i].id, "", 3) == 0) strncpy(vars[i].id, varID, 3);
    vars[i].value = value;
  }

  void initVars() {
    for (int i=0; i< nrOfAppVars; i++) {
      strncpy(vars[i].id, "", 3);
      vars[i].value = 0;
    }
  }

};

//note: changing SysData and AppData sizes: all instances should have the same version so change with care

struct NodeInfo {
  IPAddress ip;
  char name[32];
  uint32_t version;
  unsigned long timeStamp; //when was the package received, used to check on aging
  SysData sys;
  AppData app; //total 80

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
  UDPWLEDMessage header; // 44 bytes fixed!
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

class UserModInstances:public SysModule {

public:

  std::vector<NodeInfo> nodes;

  UserModInstances() :SysModule("Instances") {
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  void addTblRow(JsonVariant rows, std::vector<NodeInfo>::iterator node) {
    JsonArray row = rows.createNestedArray();
    row.add((char *)node->name);
    char urlString[32] = "http://";
    strncat(urlString, node->ip.toString().c_str(), sizeof(urlString)-1);
    row.add((char *)urlString);  //create a copy!
    row.add((char *)node->ip.toString().c_str());
    // row.add(node->timeStamp / 1000);

    row.add(node->sys.type?"StarMod":"WLED");

    row.add(node->version);
    row.add(node->sys.upTime);

    mdl->findVars("stage", true, [node, row](JsonObject var) { //findFun
      //look for value in node
      int value = node->app.getVar(var["id"]);
      // USER_PRINTF("insTbl %s %s: %d\n", node->name, varID, value);
      row.add(value);
    });
  }

  //setup filesystem
  void setup() {
    SysModule::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    parentVar = ui->initModule(parentVar, name);

    JsonObject tableVar = ui->initTable(parentVar, "insTbl", nullptr, false, [this](JsonObject var) { //uiFun
    const char * varID = var["id"];
      web->addResponse(varID, "label", "Instances");
      web->addResponse(varID, "comment", "List of instances");
      JsonArray rows = web->addResponseA(varID, "data");
      for (auto node=this->nodes.begin(); node!=this->nodes.end(); ++node) {
        addTblRow(rows, node);
      }
    });
    ui->initText(tableVar, "insName", nullptr, 32, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Name");
    });
    ui->initURL(tableVar, "insLink", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Show");
    });

    ui->initText(tableVar, "insIp", nullptr, 16, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "IP");
    });
    ui->initText(tableVar, "insType", nullptr, 16, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Type");
    });
    ui->initNumber(tableVar, "insVersion", -1, 0, (unsigned long)-1, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Version");
    });
    ui->initNumber(tableVar, "insUp", -1, 0, (unsigned long)-1, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Uptime");
    });
    // ui->initNumber(tableVar, "insTime", -1, 0, (unsigned long)-1, true, [](JsonObject var) { //uiFun
    //   web->addResponse(var["id"], "label", "Timestamp");
    // });

    JsonObject currentVar;

    currentVar = ui->initSelect(parentVar, "sma", 0, false, [this](JsonObject var) { //uiFun tbd: make dropdown where value is ...ip number
      web->addResponse(var["id"], "label", "Sync Master");
      web->addResponse(var["id"], "comment", "Instance to sync from");
      JsonArray select = web->addResponseA(var["id"], "data");
      JsonArray instanceObject = select.createNestedArray();
      instanceObject.add(0);
      instanceObject.add("no sync");
      for (auto node=nodes.begin(); node!=nodes.end(); ++node) {
        char option[32] = { 0 };
        strncpy(option, node->ip.toString().c_str(), sizeof(option)-1);
        strncat(option, " ", sizeof(option)-1);
        strncat(option, node->name, sizeof(option)-1);
        instanceObject = select.createNestedArray();
        instanceObject.add(node->ip[3]);
        instanceObject.add(option);
      }
    }); //syncMaster
    currentVar["stage"] = true;

    //find stage variables and add them to the table
    mdl->findVars("stage", true, [tableVar, this](JsonObject var) { //findFun

      USER_PRINTF("stage %s %s found\n", var["id"].as<const char *>(), var["value"].as<String>().c_str());

      char columnVarID[32] = "ins";
      strcat(columnVarID, var["id"]);
      JsonObject newVar; // = ui->cloneVar(var, columnVarID, [this, var](JsonObject newVar){});

      //create a var of the same type. InitVar is not calling chFun which is good in this situation!
      newVar = ui->initVar(tableVar, columnVarID, var["type"], false, nullptr, [this, var](JsonObject newVar, uint8_t rowNr) { //chFun

        if (rowNr != uint8Max) {
          //if this node update directly, otherwise send over network
          if (nodes[rowNr].ip == WiFi.localIP()) {
            mdl->setValueI(var["id"], newVar["value"][rowNr].as<uint8_t>());
          } else {
            // https://randomnerdtutorials.com/esp32-http-get-post-arduino/
            HTTPClient http;
            char serverPath[32];
            print->fFormat(serverPath, sizeof(serverPath)-1, "http://%s/json", nodes[rowNr].ip.toString().c_str());
            http.begin(serverPath);
            http.addHeader("Content-Type", "application/json");
            char postMessage[32];
            print->fFormat(postMessage, sizeof(postMessage)-1, "{\"%s\":%d}", var["id"].as<const char *>(), newVar["value"][rowNr].as<uint8_t>());

            USER_PRINTF("json post %s %s\n", serverPath, postMessage);

            int httpResponseCode = http.POST(postMessage);

            if (httpResponseCode>0) {
              Serial.print("HTTP Response code: ");
              Serial.println(httpResponseCode);
              String payload = http.getString();
              Serial.println(payload);
            }
            else {
              Serial.print("Error code: ");
              Serial.println(httpResponseCode);
            }
            // Free resources
            http.end();
          }
        }
        else {
          USER_PRINTF(" no rowNr!!");
        }
        print->printJson(" ", var);

      });

      if (newVar) {
        if (!var["min"].isNull()) newVar["min"] = var["min"];
        if (!var["max"].isNull()) newVar["max"] = var["max"];
        newVar["uiFun"] = var["uiFun"]; //copy the uiFun
      }

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
    if (SysModules::isConnected && isEnabled) {
      udpConnected = notifierUdp.begin(notifierUDPPort); //sync
      udp2Connected = instanceUDP.begin(instanceUDPPort); //nodes
    } else {
      udpConnected = false;
      udp2Connected = false;
      nodes.clear();
      ui->processUiFun("insTbl");
      //udp off ??
    }
  }

  void loop() {
    // SysModule::loop();

    handleNotifications();

    if (ui->stageVarChanged) {
      ui->stageVarChanged = false;
      sendSysInfoUDP(); 
    }
  }

  void loop10s() {
    sendSysInfoUDP(); 
  }

  void handleNotifications()
  {
    if(!SysModules::isConnected) return;

    // instanceUDP.flush(); //tbd: test if needed

    int packetSize;

    if (udpConnected) {
      int packetSize = notifierUdp.parsePacket();

      //handle sync from WLED
      if (packetSize > 0) {
        IPAddress remoteIp = notifierUdp.remoteIP();

        // USER_PRINTF("handleNotifications sync ...%d %d\n", remoteIp[3], packetSize);

        UDPWLEDSyncMessage wledSyncMessage;
        uint8_t *udpIn = (uint8_t *)&wledSyncMessage;
        notifierUdp.read(udpIn, packetSize);

        // for (int i=0; i<40; i++) {
        //   Serial.printf("%d: %d\n", i, udpIn[i]);
        // }

        USER_PRINTF("   %d %d p:%d\n", wledSyncMessage.bri, wledSyncMessage.mainsegMode, packetSize);

        std::vector<NodeInfo>::iterator node = findNode(remoteIp); //if not exist, created

        node->sys.upTime = (wledSyncMessage.timebase[0] * 256*256*256 + 256*256*wledSyncMessage.timebase[1] + 256*wledSyncMessage.timebase[2] + wledSyncMessage.timebase[3]) / 1000;
        node->sys.syncMaster = wledSyncMessage.syncGroups; //tbd: change
        
        uint8_t syncMaster = mdl->getValue("sma");
        if (syncMaster == remoteIp[3]) {
          if (node->app.getVar("bri") != wledSyncMessage.bri) mdl->setValueI("bri", wledSyncMessage.bri);
          //only set brightness
        }

        node->app.setVar("bri", wledSyncMessage.bri);
        node->app.setVar("fx", wledSyncMessage.mainsegMode); //tbd: rowNr
        node->app.setVar("pal", wledSyncMessage.palette); //tbd: rowNr

        // for (size_t x = 0; x < packetSize; x++) {
        //   char xx = (char)udpIn[x];
        //   Serial.print(xx);
        // }
        // Serial.println();

        USER_PRINTF("insTbl handleNotifications %d\n", remoteIp[3]);
        ui->processUiFun("insTbl");

        return;
      }
    }
 
    //handle nodes update
    if (udp2Connected) {
      packetSize = instanceUDP.parsePacket();

      if (packetSize > 0) {
        IPAddress remoteIp = instanceUDP.remoteIP();
        // USER_PRINTF("handleNotifications nodes ...%d %d check %d or %d\n", remoteIp[3], packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage));

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
          //read the rest of the data (flush)
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
        USER_PRINTF("insTbl remove inactive nodes %d\n", node->ip[3]);
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
    if(!SysModules::isConnected) return;
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
    starModMessage.header.version = atoi(sys->version);
    starModMessage.sys.type = 1; //StarMod
    starModMessage.sys.upTime = millis()/1000;
    starModMessage.sys.syncMaster = mdl->getValue("sma");
    starModMessage.sys.dmx.universe = 0;
    starModMessage.sys.dmx.start = 0;
    starModMessage.sys.dmx.count = 0;
    #ifdef USERMOD_E131
      if (e131mod->isEnabled) {
        starModMessage.sys.dmx.universe = mdl->getValue("dun");
        starModMessage.sys.dmx.start = mdl->getValue("dch");
        starModMessage.sys.dmx.count = 3;//e131->varsToWatch.size();
      }
    #endif

    //stage values default 0
    starModMessage.app.initVars();

    //send stage values
    mdl->findVars("stage", true, [&starModMessage](JsonObject var) { //uiFun
      starModMessage.app.setVar(var["id"], var["value"]);
    });

    updateNode(starModMessage); //temp? to show own node in list as instance is not catching it's own udp message...

    IPAddress broadcastIP(255, 255, 255, 255);
    if (0 != instanceUDP.beginPacket(broadcastIP, instanceUDPPort)) {  // WLEDMM beginPacket == 0 --> error
      USER_PRINTF("sendSysInfoUDP %s s:%d p:%d i:...%d\n", (uint8_t*)&starModMessage, sizeof(UDPStarModMessage), instanceUDPPort, ip[3]);
      // for (size_t x = 0; x < sizeof(UDPWLEDMessage) + sizeof(SysData) + sizeof(AppData); x++) {
      //   char * xx = (char *)&starModMessage;
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
    for (auto node=nodes.begin(); node!=nodes.end(); ++node)
    {
      if (node->ip == ip)
        found = true;
    }

    // USER_PRINTF("updateNode Instance: ...%d n:%s %d\n", ip[3], udpStarMessage.header.name, found);

    if (!found) { //new node
      NodeInfo node;
      node.ip = ip;
      if (udpStarMessage.sys.type == 0) {//WLED only
        node.sys.type = 0; //WLED
        //updated in udp sync message:
        node.sys.upTime = 0;
        node.sys.dmx.universe = 0;
        node.sys.dmx.start = 0;
        node.sys.dmx.count = 0;
        node.sys.syncMaster = 0;
        //stage values default 0
        node.app.initVars();
      }

      nodes.push_back(node);
      std::sort(nodes.begin(),nodes.end(), [](NodeInfo &a, NodeInfo &b){ return a.ip < b.ip; });//Sorting the vector strcmp(a.name,b.name);
    }

    //iterate vector pointers so we can update the nodes
    for (std::vector<NodeInfo>::iterator node=nodes.begin(); node!=nodes.end(); ++node) {
      if (node->ip == ip) {
        node->timeStamp = millis(); //update timestamp
        strncpy(node->name, udpStarMessage.header.name, sizeof(node->name)-1);
        node->version = udpStarMessage.header.version;
        if (udpStarMessage.sys.type == 1) {//StarMod only
          node->sys = udpStarMessage.sys;

          //check for syncing
          uint8_t syncMaster = mdl->getValue("sma");
          if (syncMaster == ip[3]) {

            //find matching var
            for (int i=0; i< nrOfAppVars; i++) {
              //set node kv

              VarData newVar = udpStarMessage.app.vars[i];

              if (strncmp(newVar.id, "", 3) != 0) {

                int value = node->app.getVar(newVar.id);

                //if no value found or value has changed
                if (value == -1 || value != newVar.value) {
                  char varID[4];
                  strncpy(varID, newVar.id, 3);
                  varID[3]='\0';

                  USER_PRINTF("AppData3 %s %s %d\n", node->name, varID, newVar.value);
                  mdl->setValueI(varID, newVar.value);
                }
              }
            }
          } 

          //set values to node
          for (int i=0; i< nrOfAppVars; i++)
            node->app.vars[i] = udpStarMessage.app.vars[i];
        }

        //only update cell in instbl!
        //create a json string
        //send the json
        //ui to parse the json

        if (found) {
          JsonDocument *responseDoc = web->getResponseDoc();
          responseDoc->clear(); //needed for deserializeJson?
          JsonVariant responseVariant = responseDoc->as<JsonVariant>();

          JsonArray rows = web->addResponseA("updrow", "insTbl");
          addTblRow(rows, node);

          web->sendDataWs(responseVariant); //send to all clients

          // print->printJson("updateNode updrow", responseVariant);
        }

      } //ip
    }
    if (!found) {
      ui->processUiFun("ddpInst"); //show the new instance in the dropdown  
      ui->processUiFun("artInst"); //show the new instance in the dropdown  

      USER_PRINTF("insTbl updateNode %d\n", ip[3]);
      ui->processUiFun("insTbl");
    }

  }

  std::vector<NodeInfo>::iterator findNode( IPAddress ip) {

    bool found = false;
    for (auto node=nodes.begin(); node!=nodes.end(); ++node)
    {
      if (node->ip == ip)
        found = true;
    }

    if (!found) { //node always found
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

};

static UserModInstances *instances;