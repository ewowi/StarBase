/*
   @title     StarBase
   @file      SysModInstances.h
   @date      20241219
   @repo      https://github.com/ewoudwijma/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewoudwijma/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#ifdef STARBASE_USERMOD_E131
  #include "../User/UserModE131.h"
#endif
#include "SysModSystem.h"
#include "SysModNetwork.h" //for localIP
#include "SysModules.h"

struct DMX {
  byte universe:3; //3 bits / 8
  uint16_t start:9; //9 bits / 512
  byte count:4; // 4 bits / 16
}; //total 16 bits

//additional data not in wled header
struct SysData {
  unsigned long uptime;
  uint32_t now; //25
  uint8_t timeSource; //29
  uint32_t tokiTime; //30 in sec
  uint16_t tokiMs; //34
  byte type; //0=WLED, 1=StarBase, 2=StarLight, 3=StarLedsLive, else=StarFork
  DMX dmx;
  uint8_t macAddress[6]; // 48 bits WIP
};

struct VarData {
  char id[3];
  byte value;
}; //4

//note: changing SysData and jsonData sizes: all instances should have the same version so change with care

struct InstanceInfo {
  IPAddress ip;
  char name[32];
  uint32_t version; //release/version date build
  unsigned long timeStamp; //when was the package received, used to check on aging
  SysData sysData;
  JsonDocument jsonData;
};

struct UDPWLEDMessage {
  byte token;       //0: 'binary token 255'
  byte id;          //1: id '1'
  byte ip0;         //2: uint32_t ip takes 6 bytes instead of 4 ?!?! so splitting it into bytes
  byte ip1;         //3
  byte ip2;         //4
  byte ip3;         //5
  char name[32];    //6..37: server name
  byte type;        //38: instance type id //esp32 tbd: CONFIG_IDF_TARGET_ESP32S3 etc
  byte insId;      //39: instance id  unit ID == last IP number
  uint32_t version; //40..43: version ID (here it takes 4 bytes (as it should)
}; //total 44 bytes!

//compatible with WLED instances as it only interprets first 44 bytes
struct UDPStarMessage {
  UDPWLEDMessage header; // 44 bytes fixed!
  SysData sysData;
  char jsonString[1460 - sizeof(UDPWLEDMessage) - sizeof(SysData)];
};

//WLED syncmessage 1193 bytes
struct UDPWLEDSyncMessage { //see notify( in WLED
  byte protocol; //0
  byte callMode; //1
  byte bri; //2 LEDs specific
  byte rCol0; //3
  byte gCol0; //4
  byte bCol0; //5
  byte nightlightActive; //6
  byte nightlightDelayMins; //7
  byte mainsegMode; //8
  byte mainsegSpeed; //9
  byte wCol0; //10
  byte version; //11 compatibilityVersionByte - we assume 12 only (toki since >7)
  byte col1[4]; //12
  byte mainsegIntensity; //16
  byte transitionDelay[2]; //17
  byte palette; //19
  byte col2[4]; //20
  byte followUp; //24
  byte now[4]; //25
  byte timeSource; //29
  byte tokiTime[4]; //30
  byte tokiMs[2]; //34
  byte syncGroups; //36
  char body[1193 - 37]; //41 +(32*36)+0 = 1193
};

class SysModInstances:public SysModule {

public:

  std::vector<InstanceInfo> instances;
  std::vector<JsonObject> changedVarsQueue;

  SysModInstances() :SysModule("Instances") {
  };

  void setup() override {
    SysModule::setup();

    const Variable parentVar = ui->initSysMod(Variable(), name, 3000);

    Variable tableVar = ui->initTable(parentVar, "instances", nullptr, true);
    
    ui->initText(tableVar, "name", nullptr, 32, false, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(JsonString(instances[rowNrL].name), rowNrL);
        return true;
      // comment this out for the time being as causes corrupted instance names
      // case onChange:
      //   strlcpy(instances[rowNr].name, variable.getValue(rowNr), sizeof(instances[rowNr].name));
      //   sendMessageUDP(instances[rowNr].ip, "name", variable.getValue(rowNr));
      //   return true;
      default: return false;
    }});

    ui->initURL(tableVar, "show", nullptr, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++) {
          char urlString[32] = "http://";
          strlcat(urlString, instances[rowNrL].ip.toString().c_str(), sizeof(urlString));
          variable.setValue(JsonString(urlString), rowNrL);
        }
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "link", UINT16_MAX, 0, UINT16_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(calcGroup(instances[rowNrL].name), rowNrL);
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "IP", nullptr, 16, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(JsonString(instances[rowNrL].ip.toString().c_str()), rowNrL);
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "type", nullptr, 16, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++) {
          byte type = instances[rowNrL].sysData.type;
          variable.setValue((type==0)?"WLED":(type==1)?"StarBase":(type==2)?"StarLight":(type==3)?"StarLedsLive":"StarFork", rowNrL);
        }
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "version", UINT16_MAX, 0, (unsigned long)-1, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(instances[rowNrL].version, rowNrL);
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "uptime", UINT16_MAX, 0, (unsigned long)-1, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(instances[rowNrL].sysData.uptime, rowNrL);
        return true;
      default: return false;
    }});
    ui->initNumber(tableVar, "now", UINT16_MAX, 0, (unsigned long)-1, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(instances[rowNrL].sysData.now / 1000, rowNrL);
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "timestamp", UINT16_MAX, 0, (unsigned long)-1, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(instances[rowNrL].sysData.timeSource, rowNrL);
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "time", UINT16_MAX, 0, (unsigned long)-1, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(instances[rowNrL].sysData.tokiTime, rowNrL);
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "ms", UINT16_MAX, 0, (unsigned long)-1, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          variable.setValue(instances[rowNrL].sysData.tokiMs, rowNrL);
        return true;
      default: return false;
    }});

    //find dash variables and add them to the table
    mdl->findVars("dash", true, [tableVar, this](Variable variable) { //findFun

      ppf("dash %s.%s %s found\n", variable.pid(), variable.id(), variable.valueString().c_str());
      // dash Fixture.on 1 found
      // dash Fixture.brightness 94 found
      // dash layers.effect [30] found
      // dash layers.projection [1] found
      // dash E131.channel 1 found

      //todo: add pid to it
      char columnVarID[64];
      print->fFormat(columnVarID, sizeof(columnVarID), "ins%s_%s", variable.pid(), variable.id());

      //create a var of the same type. InitVar is not calling onChange which is good in this situation!  // = ui->cloneVar(var, columnVarID, [this, var](JsonObject insVar){});
      Variable insVariable = mdl->initVar(tableVar, columnVarID, variable.var["type"], false, [this](Variable insVariable, uint8_t rowNr, uint8_t eventType) {
        //extract the variable from insVariable.id()
        char pid[32]; strlcpy(pid, insVariable.id() + 3, sizeof(pid)); //+3 : remove ins
        char * id = strtok(pid, "_"); if (id != nullptr ) {strlcpy(pid, id, sizeof(pid)); id = strtok(nullptr, "_");} //split pid and id
        Variable variable = Variable(pid, id); 
        switch (eventType) { //varEvent
        case onSetValue:
          //should not trigger onChange
          for (size_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++) {
            // ppf("initVar dash %s[%d]\n", variable.id(), rowNrL);
            //do what setValue is doing except calling onChange
            // insVar["value"][rowNrL] = instances[rowNrL].jsonData[variable.id()]; //only int values...
            JsonVariant value = instances[rowNrL].jsonData[variable.id()];
            web->addResponse(insVariable.var, "value", value, rowNrL); // error: passing 'const Variable' as 'this' argument discards qualifiers

            // mdl->setValue(insVariable.var, instances[rowNrL].jsonData[variable.id()], rowNr);
          //send to ws?
          }
          return true;
        case onUI:
          // call onUI of the base variable for the new variable
          if (variable.var["fun"].as<uint8_t>() != UINT8_MAX)
            mdl->varEvents[variable.var["fun"]](insVariable, rowNr, onUI);
          else
            insVariable.publish(onUI, rowNr); //is insVariable subscribed ???
          return true;
        case onChange: {
          //do not set this initially!!!
          if (rowNr != UINT8_MAX) {
            //if this instance update directly, otherwise send over network
            if (instances[rowNr].ip == net->localIP()) {
              variable.setValue(insVariable.getValue(rowNr).as<uint8_t>()); //this will call sendDataWS (tbd...), do not set for rowNr
            } else {
              sendMessageUDP(instances[rowNr].ip, variable.var, insVariable.getValue(rowNr));
            }
          }
          // print->printJson(" ", var);
          return true; }
        default: return false;
      }});

      if (insVariable.var) {
        if (!variable.var["min"].isNull()) insVariable.var["min"] = variable.var["min"];
        if (!variable.var["max"].isNull()) insVariable.var["max"] = variable.var["max"];
        if (!variable.var["log"].isNull()) insVariable.var["log"] = variable.var["log"];
        // insVar["fun"] = var["fun"]; //copy the onUI
      }

    }); //findVars

    if (sizeof(UDPWLEDMessage) != 44) {
      ppf("dev Size of UDP message is not 44: %d\n", sizeof(UDPWLEDMessage));
      // ppf("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }
    if (sizeof(UDPStarMessage) != 1460) { //size of UDP Packet
      // one udp frame should be 1460, not 1472 (then split by network in 1460 and 12)
      ppf("dev Size of UDP message is not 44: %d\n", sizeof(UDPStarMessage));
      // ppf("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }

    ppf("UDP message sizes WLED:%d Star:%d WLED-Sync:%d\n", sizeof(UDPWLEDMessage), sizeof(UDPStarMessage), sizeof(UDPWLEDSyncMessage));
  }

  void onOffChanged() override {
    if (mdls->isConnected && isEnabled) {
      udpConnected = notifierUdp.begin(notifierUDPPort); //sync
      udp2Connected = instanceUDP.begin(instanceUDPPort); //instances
    } else {
      udpConnected = false;
      udp2Connected = false;
      instances.clear();

      //not needed here as there is no connection
      // ui->processOnUI("instances");

      //udp off ??
    }
  }

  void loop20ms() override { //20 ms instead of loop() tripples the loops/sec!

    handleNotifications();

    while (changedVarsQueue.size()) {
      JsonObject var = changedVarsQueue.front();
      sendMessageUDP(IPAddress(255, 255, 255, 255), var, var["value"]); //broadcast
      changedVarsQueue.erase(changedVarsQueue.begin());
    }

  }

  void loop10s() override {
    sendSysInfoUDP();  //temporary every second
  }

  //distract the groupName of an instance name
  bool groupOfName(const char *name, char *group = nullptr) {
    char copy[32];
    strlcpy(copy, name, sizeof(copy));

    char * token = strtok(copy, "-"); //before minus

    //check if group
    if (token != nullptr) {
      // ppf("groupOfName 1:%s 2:%s 3:%s 4:%s 5:%s\n",name, strnstr(name, "-", 32), name, strtok(name, "-"), name?name:"X"); 
      token = strtok(nullptr, "-"); //after -
      if (token != nullptr) {
        // ppf("groupOfName g:%s i:%s\n", copy, token); 
        if (group) strlcpy(group, copy, sizeof(copy)); //group has same size!
        return true;
      }
    }
    return false;

  }

  // identify if an instance belongs to a group: 0: no, 1: group of 1, >1: group with more
  uint8_t calcGroup(const char * insName) {
    uint8_t calc = 0;
    for (InstanceInfo &instance: instances) {
      char group1[32];
      char group2[32];
      if (groupOfName(instance.name, group1) && groupOfName(insName, group2) && strncmp(group1, group2, sizeof(group1)) == 0)
        calc++;
    }
    return calc++;
  }

  #define PRESUMED_NETWORK_DELAY 3 //how many ms could it take on avg to reach the receiver? This will be added to transmitted times

  void handleNotifications()
  {
    if(!mdls->isConnected) return;

    // instanceUDP.flush(); //tbd: test if needed

    int packetSize;

    //handle sync from WLED
    if (udpConnected) {
      int packetSize = notifierUdp.parsePacket();

      if (packetSize > 0) {
        // IPAddress remoteIp = notifierUdp.remoteIP();

        if (packetSize == sizeof(UDPWLEDSyncMessage)) { //1193 bytes

          ppf("handleNotifications WLED sync ...%d %d %d\n", notifierUdp.remoteIP()[3], packetSize, sizeof(UDPWLEDSyncMessage));

          UDPWLEDSyncMessage wledSyncMessage;
          byte *udpIn = (byte *)&wledSyncMessage;
          notifierUdp.read(udpIn, packetSize);

          // for (int i=0; i<40; i++) {
          //   Serial.printf("%d: %d\n", i, udpIn[i]);
          // }

          ppf("   %d %d p:%d\n", wledSyncMessage.bri, wledSyncMessage.mainsegMode, packetSize); //LEDs specific

          InstanceInfo *instance = findInstance(notifierUdp.remoteIP()); //if not exist, created

          // instance->sysData.uptime = (wledSyncMessage.now[0] * 256*256*256 + 256*256*wledSyncMessage.now[1] + 256*wledSyncMessage.now[2] + wledSyncMessage.now[3]) / 1000;
          instance->sysData.uptime = (wledSyncMessage.now[0] << 24) | (wledSyncMessage.now[1] << 16) | (wledSyncMessage.now[2] << 8) | (wledSyncMessage.now[3]);
          instance->sysData.now = (wledSyncMessage.now[0] << 24) | (wledSyncMessage.now[1] << 16) | (wledSyncMessage.now[2] << 8) | (wledSyncMessage.now[3]);
          instance->sysData.timeSource = wledSyncMessage.timeSource;
          instance->sysData.tokiTime = (wledSyncMessage.tokiTime[0] << 24) | (wledSyncMessage.tokiTime[1] << 16) | (wledSyncMessage.tokiTime[2] << 8) | (wledSyncMessage.tokiTime[3]);
          instance->sysData.tokiMs = (wledSyncMessage.tokiMs[0] << 8) | (wledSyncMessage.tokiMs[1]);

          //don't update toki for WLED ATM

              // uint32_t t = instance->sysData.now;
              // t += PRESUMED_NETWORK_DELAY; //adjust trivially for network delay
              // t -= millis();
              // sys->timebase = t;
              // // timebaseUpdated = true;

              // Toki::Time tm;
              // tm.sec = instance->sysData.tokiTime;
              // tm.ms = instance->sysData.tokiMs;
              // if (instance->sysData.timeSource > sys->toki.getTimeSource()) { //if sender's time source is more accurate
              //   sys->toki.adjust(tm, PRESUMED_NETWORK_DELAY); //adjust trivially for network delay
              //   uint8_t ts = TOKI_TS_UDP;
              //   if (instance->sysData.timeSource > 99) ts = TOKI_TS_UDP_NTP;
              //   else if (instance->sysData.timeSource >= TOKI_TS_SEC) ts = TOKI_TS_UDP_SEC;
              //   sys->toki.setTime(tm, ts);
              // } else if (/*timebaseUpdated && */ sys->toki.getTimeSource() > 99) { //if we both have good times, get a more accurate timebase
              //   Toki::Time myTime = sys->toki.getTime();
              //   uint32_t diff = sys->toki.msDifference(tm, myTime);
              //   sys->timebase -= PRESUMED_NETWORK_DELAY; //no need to presume, use difference between NTP times at send and receive points
              //   if (sys->toki.isLater(tm, myTime)) {
              //     sys->timebase += diff;
              //   } else {
              //     sys->timebase -= diff;
              //   }
              // }
          
          //LEDs specific
          instance->jsonData["brightness"] = wledSyncMessage.bri; 
          instance->jsonData["effect"] = wledSyncMessage.mainsegMode; //tbd: rowNr
          instance->jsonData["palette"] = wledSyncMessage.palette; //tbd: rowNr

          // for (size_t x = 0; x < packetSize; x++) {
          //   char xx = (char)udpIn[x];
          //   Serial.print(xx);
          // }
          // Serial.println();

          ppf("instances handleNotifications %d\n", notifierUdp.remoteIP()[3]);
          for (JsonObject childVar: Variable("Instances", "instances").children())
            Variable(childVar).triggerEvent(onSetValue); //set the value (WIP) ); //rowNr //instance - instances.begin()

          web->recvUDPCounter++;
          web->recvUDPBytes+=packetSize;
        }
        else
          ppf("dev WLED sync massage not size %d\n", sizeof(UDPWLEDSyncMessage));

        return;
      }
    }
 
    //handle instances update
    if (udp2Connected) {
      packetSize = instanceUDP.parsePacket();

      if (packetSize > 0) {
        // IPAddress remoteIp = instanceUDP.remoteIP();
        // ppf("handleNotifications instances ...%d %d check %d or %d\n", instanceUDP.remoteIP()[3], packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarMessage));

        bool found = false;

        if (packetSize == sizeof(UDPWLEDMessage)) { //WLED instance
          UDPStarMessage starMessage;
          byte *udpIn = (byte *)&starMessage.header;
          instanceUDP.read(udpIn, packetSize);

          // ppf("WLED instance %s received: size: %d\n", instanceUDP.remoteIP().toString().c_str(), packetSize);
          // for (int i=0; i<44; i++) {
          //   Serial.printf("%d: %d\n", i, udpIn[i]);
          // }

          starMessage.sysData.type = 0; //WLED

          if (starMessage.header.ip0 == net->localIP()[0]) { // checksum - no other type of message
            updateInstance(starMessage);
            found = true;
          }
        }

        if (!found && packetSize == sizeof(UDPStarMessage)) { //StarBase instance
          UDPStarMessage starMessage;
          byte *udpIn = (byte *)&starMessage;
          instanceUDP.read(udpIn, packetSize);

          // ppf("Star instance %s received: size: %d\n", instanceUDP.remoteIP().toString().c_str(), packetSize);

          if (starMessage.header.ip0 == net->localIP()[0]) { // checksum - no other type of message
            updateInstance(starMessage);
            found = true;
          }
        }

        if (!found) { // check on json
          char buffer[packetSize];
          instanceUDP.read(buffer, packetSize);

          JsonDocument message;
          DeserializationError error = deserializeJson(message, buffer);
          if (error)
            ppf("handleNotifications i:%d no json l: %d e:%s\n", instanceUDP.remoteIP()[3], strnlen(buffer, packetSize), error.c_str());
          else {
            if (instanceUDP.remoteIP()[3] != net->localIP()[3]) { //only others

              InstanceInfo *instance = findInstance(instanceUDP.remoteIP()); //if not exist, created
              char group1[32];
              char group2[32];
              if (groupOfName(instance->name, group1) && groupOfName(mdl->getValue("System", "name"), group2) && strncmp(group1, group2, sizeof(group1)) == 0) {
                  if (!message["id"].isNull() && !message["value"].isNull()) {
                    ppf("handleNotifications i:%d json message %.*s l:%d\n", instanceUDP.remoteIP()[3], packetSize, buffer, packetSize);

                    Variable(message["pid"].as<const char *>(), message["id"].as<const char *>()).setValueJV(message["value"]);
                  }
                }
              }
            else
              ppf("handleNotifications self i:%d b:%.*s\n", instanceUDP.remoteIP()[3], packetSize, buffer);
          }
        }

        web->recvUDPCounter++;
        web->recvUDPBytes+=packetSize;

      } //packetSize
      else {
      }
    } //udp2Connected

    //remove inactive instances
    bool erased = false;
    for (std::vector<InstanceInfo>::iterator instance=instances.begin(); instance!=instances.end(); ) {
      if (millis() - instance->timeStamp > 32000) { //assuming a ping each 30 seconds
        instance = instances.erase(instance);
        erased = true;
      }
      else
        ++instance;
    }
    if (erased) {
      ppf("instances remove inactive instances\n");
      for (JsonObject childVar: Variable("Instances", "instances").children())
        Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)); //no rowNr so all rows updated

      //tbd: pubsub mechanism
      //LEDs specific
      Variable("DDP", "instance").triggerEvent(onUI); //rebuild options
      // Variable("Artnet", "artInst").triggerEvent(onUI); //rebuild options
    }
  }

  void sendSysInfoUDP()
  {
    if(!mdls->isConnected) return;
    if (!udp2Connected) return;

    IPAddress localIP = net->localIP();
    if (!localIP || localIP == IPAddress(255,255,255,255)) localIP = IPAddress(4,3,2,1);

    UDPStarMessage starMessage;
    starMessage.header.token = 255; //WLED only accepts 255
    starMessage.header.id = 1; //WLED only accepts 1
    starMessage.header.ip0 = localIP[0];
    starMessage.header.ip1 = localIP[1];
    starMessage.header.ip2 = localIP[2];
    starMessage.header.ip3 = localIP[3];
    memset((byte *)starMessage.header.name, 0, 32); //init with 0
    const char * name = mdl->getValue("System", "name");
    strlcpy(starMessage.header.name, name?name:_INIT(TOSTRING(APP)), sizeof(starMessage.header.name));
    #if defined(CONFIG_IDF_TARGET_ESP32S2)
      starMessage.header.type = 33;
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
      starMessage.header.type = 34;
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
      starMessage.header.type = 35;
    #elif defined(ESP32)
      starMessage.header.type = 32;
    #else //???
      prf("dev unknown board\n");
      starMessage.header.type = 0;
    #endif
    // if (bri) 
      starMessage.header.type |= 0x80U;  // add on/off state
    // 38: 160
    // 39: 171
    // 40: 138
    // 41: 186
    // 42: 36
    // 43: 0
    starMessage.header.insId = localIP[3]; //WLED: used in map of instances as index!
    starMessage.header.version = VERSION;
    starMessage.sysData.type = (strncmp(_INIT(TOSTRING(APP)), "StarBase", 9)==0)?1:(strncmp(_INIT(TOSTRING(APP)), "StarLight", 10)==0)?2:(strncmp(_INIT(TOSTRING(APP)), "StarLedsLive", 13)==0)?3:99; //0=WLED, 1=StarBase, 2=StarLight, 3=StarLedsLive, else=StarFork
    starMessage.sysData.uptime = millis()/1000;
    starMessage.sysData.now = millis() + sys->timebase; //similar to now
    starMessage.sysData.timeSource = sys->toki.getTimeSource();
    starMessage.sysData.tokiTime = sys->toki.getTime().sec;
    starMessage.sysData.tokiMs = sys->toki.getTime().ms;
    starMessage.sysData.dmx.universe = 0;
    starMessage.sysData.dmx.start = 0;
    starMessage.sysData.dmx.count = 0;
    #ifdef STARBASE_USERMOD_E131
      if (e131mod->isEnabled) {
        starMessage.sysData.dmx.universe = mdl->getValue("E131", "universe");
        starMessage.sysData.dmx.start = mdl->getValue("E131", "channel");
        starMessage.sysData.dmx.count = 3;//e131->varsToWatch.size();
      }
    #endif

    updateInstance(starMessage); //temp? to show own instance in list as instance is not catching it's own udp message...

    //other way around: first set instance variables, then fill starMessage
    for (InstanceInfo &instance: instances) {
      if (instance.ip == net->localIP()) {
        instance.jsonData.to<JsonObject>(); //clear

        //send dash values
        mdl->findVars("dash", true, [&instance](Variable variable) { //varEvent
          instance.jsonData[variable.id()] = variable.value();
          // // print->printJson("setVar", var);
          // JsonArray valArray = variable.valArray();
          // if (valArray.isNull())
          // else if (valArray.size())
          //   instance.jsonData[variable.id()] = valArray;
        });

        serializeJson(instance.jsonData, starMessage.jsonString);
        // ppf("sendSysInfoUDP ip:%d s:%s\n", instance.ip[3], starMessage.jsonString);
        // print->printJson(" d:", instance.jsonData);
        // print->printJDocInfo("   info", instance.jsonData);
      }
    }

    // broadcast to network
    if (0 != instanceUDP.beginPacket(IPAddress(255, 255, 255, 255), instanceUDPPort)) {  // WLEDMM beginPacket == 0 --> error
      // ppf("sendSysInfoUDP %s s:%d p:%d i:...%d\n", starMessage.header.name, sizeof(UDPStarMessage), instanceUDPPort, localIP[3]);
      // for (size_t x = 0; x < sizeof(UDPWLEDMessage) + sizeof(SysData); x++) {
      //   char * xx = (char *)&starMessage;
      //   Serial.printf("%d: %d - %c\n", x, xx[x], xx[x]);
      // }

      instanceUDP.write((byte*)&starMessage, sizeof(UDPStarMessage));
      web->sendUDPCounter++;
      web->sendUDPBytes+=sizeof(UDPStarMessage);
      instanceUDP.endPacket();
    }
    else {
      ppf("sendSysInfoUDP error\n");
    }
    // if (0 != instanceUDP.beginPacket(IPAddress(255, 255, 255, 255), instanceUDPPort)) {  // WLEDMM beginPacket == 0 --> error
    //   ppf("sendSysInfoUDP %s s:%d p:%d i:...%d\n", starMessage.header.name, sizeof(UDPWLEDMessage), instanceUDPPort, localIP[3]);
    //   for (size_t x = 0; x < sizeof(UDPWLEDMessage); x++) {
    //     char * xx = (char *)&starMessage.header;
    //     Serial.printf("%d: %d - %c\n", x, xx[x], xx[x]);
    //   }

    //   instanceUDP.write((uint8_t*)&starMessage.header, sizeof(UDPWLEDMessage));
    //   web->sendUDPCounter++;
    //   web->sendUDPBytes+=sizeof(UDPWLEDMessage);
    //   instanceUDP.endPacket();
    // }
    // else {
    //   ppf("sendSysInfoUDP error\n");
    // }
  }

  //sends an UDP message to a specific ip. Broadcast?
  void sendMessageUDP(IPAddress ip, JsonObject var, JsonVariant value) {
    if (0 != instanceUDP.beginPacket(ip, instanceUDPPort)) {

      JsonDocument message;
      message["pid"] = var["pid"];
      message["id"] = var["id"];
      message["value"] = value;

      size_t len = measureJson(message);

      char buffer[len];

      serializeJson(message, buffer, len);

      instanceUDP.write((byte *)buffer, len);
      web->sendUDPCounter++;
      web->sendUDPBytes+=len;
      instanceUDP.endPacket();
      ppf("sendMessageUDP ip:%d b:%.*s\n", ip[3], len, buffer);
    }
  }

  void updateInstance( UDPStarMessage udpStarMessage) {
    IPAddress messageIP = IPAddress(udpStarMessage.header.ip0, udpStarMessage.header.ip1, udpStarMessage.header.ip2, udpStarMessage.header.ip3);

    bool instanceFound = false;
    for (InstanceInfo &instance : instances) {
      if (instance.ip == messageIP)
        instanceFound = true;
    }

    // ppf("updateInstance Instance: ...%d n:%s found:%d\n", messageIP[3], udpStarMessage.header.name, instanceFound);

    if (!instanceFound) { //new instance
      InstanceInfo instance;
      instance.ip = messageIP;
      if (udpStarMessage.sysData.type == 0) {//WLED only
        instance.sysData.type = 0; //WLED
        //updated in udp sync message:
        instance.sysData.uptime = 0;
        instance.sysData.dmx.universe = 0;
        instance.sysData.dmx.start = 0;
        instance.sysData.dmx.count = 0;
        //dash values default 0
        instance.jsonData.to<JsonObject>();
      }

      instances.push_back(instance);
      std::sort(instances.begin(),instances.end(), [](InstanceInfo &a, InstanceInfo &b){ return strncmp(a.name,b.name, sizeof(a.name))<0; });
    }

    //update the instance in the instances array with the message data

    // uint8_t rowNr = 0;
    for (InstanceInfo &instance: instances) {
      if (instance.ip == messageIP) {
        //update instance from StarMessage
        instance.timeStamp = millis(); //update timestamp (when was the package received)
        strlcpy(instance.name, udpStarMessage.header.name, sizeof(instance.name));
        instance.version = udpStarMessage.header.version;

        if (instance.ip == net->localIP()) {
          esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, instance.sysData.macAddress);
          // ppf("macaddress %02X:%02X:%02X:%02X:%02X:%02X\n", instance.macAddress[0], instance.macAddress[1], instance.macAddress[2], instance.macAddress[3], instance.macAddress[4], instance.macAddress[5]);
        }

        if (udpStarMessage.sysData.type >= 1) {//StarBase, StarLight and forks only
          instance.sysData = udpStarMessage.sysData;

          if (instance.ip != net->localIP()) { //send from localIP will be done after updateInstance
            char group1[32];
            char group2[32];
            if (groupOfName(instance.name, group1) && groupOfName(mdl->getValue("System", "name"), group2) && strncmp(group1, group2, sizeof(group1)) == 0) {

              uint32_t t = instance.sysData.now;
              t += PRESUMED_NETWORK_DELAY; //adjust trivially for network delay
              t -= millis();
              sys->timebase = t;
              // timebaseUpdated = true;

              Toki::Time tm;
              tm.sec = instance.sysData.tokiTime;
              tm.ms = instance.sysData.tokiMs;
              if (instance.sysData.timeSource > sys->toki.getTimeSource() || sys->toki.getTimeSource() == TOKI_TS_NONE) { //if sender's time source is more accurate
                sys->toki.adjust(tm, PRESUMED_NETWORK_DELAY); //adjust trivially for network delay
                uint8_t ts = TOKI_TS_UDP; //5
                if (instance.sysData.timeSource > 99) ts = TOKI_TS_UDP_NTP; //110
                else if (instance.sysData.timeSource >= TOKI_TS_SEC) ts = TOKI_TS_UDP_SEC; //20
                sys->toki.setTime(tm, ts);
              } else if (/*timebaseUpdated && */ sys->toki.getTimeSource() > 99) { //if we both have good times, get a more accurate timebase
                Toki::Time myTime = sys->toki.getTime();
                uint32_t diff = sys->toki.msDifference(tm, myTime);
                sys->timebase -= PRESUMED_NETWORK_DELAY; //no need to presume, use difference between NTP times at send and receive points
                if (sys->toki.isLater(tm, myTime)) {
                  sys->timebase += diff;
                } else {
                  sys->timebase -= diff;
                }
              }

              //set instance.jsonData from new string
              JsonDocument newData;
              DeserializationError error = deserializeJson(newData, udpStarMessage.jsonString);
              if (error || !newData.is<JsonObject>()) {
                // ppf("dev updateInstance json failed ip:%d e:%s\n", instance.ip[3], error.c_str(), udpStarMessage.jsonString);
                //failed because some instances not on latest firmware, so turned off temporarily (tbd/wip)
              }
              else {
                //check if instance belongs to the same group

                for (JsonPair pair: newData.as<JsonObject>()) {
                  // ppf("updateInstance sync from i:%s k:%s v:%s\n", instance.name, pair.key().c_str(), pair.value().as<String>().c_str());

                  char pid[32];
                  strlcpy(pid, pair.key().c_str(), sizeof(pid));
                  char * id = strtok(pid, ".");
                  if (id != nullptr ) {
                    strlcpy(pid, id, sizeof(pid)); //copy the id part
                    id = strtok(nullptr, "."); //the rest after .
                  }

                  Variable(pid, id).setValueJV(pair.value());
                }
                instance.jsonData = newData; // deepcopy: https://github.com/bblanchon/ArduinoJson/issues/1023
                // ppf("updateInstance json ip:%d", instance.ip[3]);
                // print->printJson(" d:", instance.jsonData);
              }
            }
          } //same group
        }

        //only update cell in instbl!
        //create a json string
        //send the json
        //ui to parse the json

        if (instanceFound) {
          // JsonObject responseObject = web->getResponseObject();

          // responseObject["updRow"]["id"] = "instances";
          // responseObject["updRow"]["rowNr"] = rowNr;
          // responseObject["updRow"]["value"].to<JsonArray>();
          // addTblRow(responseObject["updRow"]["value"], instance);

          // web->sendResponseObject();

          // ppf("updateInstance updRow\n");

          for (JsonObject childVar: Variable("Instances", "instances").children())
            Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)); //rowNr instance - instances.begin()

          //tbd: now done for all rows, should be done only for updated rows!
        }

      } //ip
      // rowNr++;
    } // for instances

    if (!instanceFound) {
      ppf("instances new instance %s\n", messageIP.toString().c_str());

      //tbd: pubsub mechanism
      //LEDs specific
      Variable("DDP", "instance").triggerEvent(onUI); //rebuild options
      // Variable(Artnet", "artInst").triggerEvent(onUI); //rebuild options

      // ui->processOnUI("instances");
      //run though it sorted to find the right rowNr
      // for (std::vector<InstanceInfo>::iterator instance=instances.begin(); instance!=instances.end(); ++instance) {
      //   if (instance->ip == messageIP) {
          for (JsonObject childVar: Variable("Instances", "instances").children()) {
            Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)); //no rowNr, update all
          }
      //   }
      // }
    }
  }

  InstanceInfo * findInstance(IPAddress ip) {

    bool instanceFound = false;
    for (InstanceInfo &instance : instances) {
      if (instance.ip == ip)
        instanceFound = true;
    }

    if (!instanceFound) { //instance always found
      InstanceInfo instance;
      instance.ip = ip;
      instances.push_back(instance);
      std::sort(instances.begin(),instances.end(), [](InstanceInfo &a, InstanceInfo &b){ return strncmp(a.name,b.name, sizeof(a.name))<0; });
    }

    // InstanceInfo foundInstance;
    //iterate vector pointers so we can update the instances
    for (InstanceInfo &instance : instances) {
      if (instance.ip == ip) {
        return &instance;
      }
    }

    return nullptr;
  }

  private:
    //sync (only WLED)
    WiFiUDP notifierUdp;
    uint16_t notifierUDPPort = 21324;
    bool udpConnected = false;

    //instances (WLED and StarBase)
    WiFiUDP instanceUDP;
    uint16_t instanceUDPPort = 65506;
    bool udp2Connected = false;

};

extern SysModInstances *instances;