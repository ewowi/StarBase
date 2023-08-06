/*
   @title     StarMod
   @file      UserModWLEDDiscovery.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

class UserModWLEDDiscovery:public Module {

public:

  UserModWLEDDiscovery() :Module("WLED Discovery") {
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
    isConnected = true;
    udp.begin(65506);
  }

  void loop(){
    // Module::loop();
    if(!isConnected) return;

    int packetSize = udp.parsePacket();

    if (packetSize) {
      IPAddress remoteIp = udp.remoteIP();
      // TODO: actually look at the contents of the packet to fetch version, name etc
      print->print("WLED: %s (%u)\n", remoteIp.toString().c_str(), packetSize);
      udp.read(packetBuffer, packetSize);
    }
  }

  private:
    bool isConnected = false;
    char packetBuffer[255];
    WiFiUDP udp;

};

static UserModWLEDDiscovery *wledDiscoveryMod;