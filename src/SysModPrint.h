#pragma once
#include "module.h"

class SysModPrint:public Module {

public:

  SysModPrint() :Module("Print") {
    print("%s %s\n", __PRETTY_FUNCTION__, name);

    Serial.begin(115200);
    delay(5000); //if (!Serial) doesn't seem to work, check with SoftHack007

    print("%s %s %s\n",__PRETTY_FUNCTION__,name, success?"success":"failed");
  };

  //setup Serial
  void setup();

  //generic print function (based on printf)
  size_t print(const char * format, ...);

  size_t println(const __FlashStringHelper * x) {
    return Serial.println(x);
  }

  void loop() {
    // Module::loop();
  }

  void printObject(JsonObject object) {
    char sep[3] = "";
    for (JsonPair pair: object) {
      print("%s%s: %s", sep, pair.key(), pair.value().as<String>().c_str());
      strcpy(sep, ", ");
    }
  }

  size_t printJson(const char * text, JsonVariantConst source) {
    Serial.printf("%s ", text);
    size_t size = serializeJson(source, Serial); //for the time being
    Serial.println();
    return size;
  }

};

static SysModPrint *print;