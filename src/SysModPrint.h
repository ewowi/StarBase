#include "Module.h"

class SysModPrint:public Module {

public:

  SysModPrint() :Module("Print") {
    print("%s %s\n", __PRETTY_FUNCTION__, name);

    Serial.begin(115200);
    delay(5000); //if (!Serial) doesn't seem to work, check with SoftHack007

    print("%s %s %s\n",__PRETTY_FUNCTION__,name, success?"success":"failed");
  };

  //setup Serial
  void setup() {
    Module::setup();
  }

  //generic print function (based on printf)
  size_t print(const char * format, ...) {
    va_list args;
    va_start(args, format);

    size_t len = vprintf(format, args);

    va_end(args);
    return len;
  }

  size_t println(const __FlashStringHelper * x) {
    return Serial.println(x);
  }

  void loop() {
    // Module::loop();
  }

  void printObject(JsonObject object) {
    char sep[3] = "";
    for (JsonPair pair: object) {
      const char * key = pair.key().c_str();
      JsonVariant value = pair.value();
      print("%s%s: %s", sep, key, value.as<String>().c_str());
      strcpy(sep, ", ");
    }
  }

};

static SysModPrint *print;