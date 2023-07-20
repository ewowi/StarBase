#include <ESPAsyncE131.h>



class UserModE131:public Module {

public:

  UserModE131() :Module("e131/sACN support") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);
  }

  void connected() {
    print->print("UserModE131::connected");
    e131 = ESPAsyncE131(universeCount);
    if (this->e131.begin(E131_MULTICAST, universe, universeCount)) { // TODO: multicast igmp failing, so only works with unicast currently
      print->print("Network exists, begin e131.begin ok\n");
      success = true;
    }
    else {
      print->print("Network exists, begin e131.begin FALED\n");
    }
    e131Created = true;
    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    if(!e131Created) {
      return;
    }
    if (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet);     // Pull packet from ring buffer
        
        Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
                htons(packet.universe),                 // The Universe for this packet
                htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                e131.stats.num_packets,                 // Packet counter
                e131.stats.packet_errors,               // Packet error counter
                packet.property_values[1]);             // Dimmer data for Channel 1

      mdl->setValue("bri", packet.property_values[1]); // TODO: ugly to have magic string 
      mdl->setValue("fx", packet.property_values[2]); // TODO: ugly to have magic string
    }
  }

  private:
    ESPAsyncE131 e131;
    boolean e131Created = false;
    int universe, universeCount = 1;

};

static UserModE131 *e131mod;