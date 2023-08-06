/*
   @title     StarMod
   @file      UserModArtNet.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

class UserModArtNet:public Module {

public:

  UserModArtNet() :Module("ArtNet") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    // calculate the number of UDP packets we need to send
    const size_t channelCount = length * (isRGBW?4:3); // 1 channel for every R,G,B,(W?) value
    const size_t ARTNET_CHANNELS_PER_PACKET = isRGBW?512:510; // 512/4=128 RGBW LEDs, 510/3=170 RGB LEDs
    const size_t packetCount = ((channelCount-1)/ARTNET_CHANNELS_PER_PACKET)+1;

    uint32_t channel = 0; 
    size_t bufferOffset = 0;

    sequenceNumber++;

    for (size_t currentPacket = 0; currentPacket < packetCount; currentPacket++) {

      if (sequenceNumber > 255) sequenceNumber = 0;

      if (!ddpUdp.beginPacket(client, ARTNET_DEFAULT_PORT)) {
        DEBUG_PRINTLN(F("Art-Net WiFiUDP.beginPacket returned an error"));
        return 1; // borked
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

      for (size_t i = 0; i < packetSize; i += (isRGBW?4:3)) {
        ddpUdp.write(scale8(buffer[bufferOffset++], bri)); // R
        ddpUdp.write(scale8(buffer[bufferOffset++], bri)); // G
        ddpUdp.write(scale8(buffer[bufferOffset++], bri)); // B
        if (isRGBW) ddpUdp.write(scale8(buffer[bufferOffset++], bri)); // W
      }

      if (!ddpUdp.endPacket()) {
        DEBUG_PRINTLN(F("Art-Net WiFiUDP.endPacket returned an error"));
        return 1; // borked
      }
      channel += packetSize;
    }

  }

};

static UserModArtNet *artnetmod;