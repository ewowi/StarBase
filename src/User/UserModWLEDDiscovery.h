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
    udp.begin(65506);
  }

  void loop(){
    // Module::loop();
    int packetSize = udp.parsePacket();

    if (packetSize) {
      IPAddress remoteIp = udp.remoteIP();
      // TODO: actually look at the contents of the packet to fetch version, name etc
      print->print("WLED: %s\n", remoteIp);
    }
  }

  private:
    WiFiUDP udp;

};

static UserModWLEDDiscovery *wledDiscoveryMod;