/*
   @title     StarMod
   @file      UserModArtNet.h
   @date      20230807
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#define ARTNET_DEFAULT_PORT 6454

static const size_t ART_NET_HEADER_SIZE = 12;
static const byte   ART_NET_HEADER[] PROGMEM = {0x41,0x72,0x74,0x2d,0x4e,0x65,0x74,0x00,0x00,0x50,0x00,0x0e};

class UserModArtNet:public Module {

public:

  IPAddress targetIp;

  UserModArtNet() :Module("ArtNet") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);
    targetIp = IPAddress(192,168,178,161); // TODO allow setting at runtime
    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void connected() {
    print->print("%s %s - Connected\n", __PRETTY_FUNCTION__, name);
    isConnected = true;
  }

  void loop(){
    // Module::loop();

    if(!isConnected) return;

    if(!lds->newFrame) return;

    // calculate the number of UDP packets we need to send
    bool isRGBW = false;

    const size_t channelCount = ledsV.nrOfLedsP * (isRGBW?4:3); // 1 channel for every R,G,B,(W?) value
    const size_t ARTNET_CHANNELS_PER_PACKET = isRGBW?512:510; // 512/4=128 RGBW LEDs, 510/3=170 RGB LEDs
    const size_t packetCount = ((channelCount-1)/ARTNET_CHANNELS_PER_PACKET)+1;

    uint32_t channel = 0; 
    size_t bufferOffset = 0;

    sequenceNumber++;

    WiFiUDP ddpUdp;

    int bri = mdl->getValue("bri");

    for (size_t currentPacket = 0; currentPacket < packetCount; currentPacket++) {

      if (sequenceNumber > 255) sequenceNumber = 0;

      if (!ddpUdp.beginPacket(targetIp, ARTNET_DEFAULT_PORT)) {
        print->print("Art-Net WiFiUDP.beginPacket returned an error\n");
        return; // borked
      }

      size_t packetSize = ARTNET_CHANNELS_PER_PACKET;

      if (currentPacket == (packetCount - 1U)) {
        // last packet
        if (channelCount % ARTNET_CHANNELS_PER_PACKET) {
          packetSize = channelCount % ARTNET_CHANNELS_PER_PACKET;
        }
      }

      byte header_buffer[ART_NET_HEADER_SIZE];
      memcpy_P(header_buffer, ART_NET_HEADER, ART_NET_HEADER_SIZE);
      ddpUdp.write(header_buffer, ART_NET_HEADER_SIZE); // This doesn't change. Hard coded ID, OpCode, and protocol version.
      ddpUdp.write(sequenceNumber & 0xFF); // sequence number. 1..255
      ddpUdp.write(0x00); // physical - more an FYI, not really used for anything. 0..3
      ddpUdp.write((currentPacket) & 0xFF); // Universe LSB. 1 full packet == 1 full universe, so just use current packet number.
      ddpUdp.write(0x00); // Universe MSB, unused.
      ddpUdp.write(0xFF & (packetSize >> 8)); // 16-bit length of channel data, MSB
      ddpUdp.write(0xFF & (packetSize     )); // 16-bit length of channel data, LSB

      for (size_t i = 0; i < ledsV.nrOfLedsP; i++) {
        CRGB pixel = ledsP[i];
        ddpUdp.write(scale8(pixel.r, bri)); // R
        ddpUdp.write(scale8(pixel.g, bri)); // G
        ddpUdp.write(scale8(pixel.b, bri)); // B
        // if (isRGBW) ddpUdp.write(scale8(buffer[bufferOffset++], bri)); // W
      }

      if (!ddpUdp.endPacket()) {
        print->print("Art-Net WiFiUDP.endPacket returned an error\n");
        return; // borked
      }
      channel += packetSize;
    }

  }

  private:
    bool isConnected = false;
    size_t sequenceNumber = 0;

};

static UserModArtNet *artnetmod;