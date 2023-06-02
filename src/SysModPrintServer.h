#include "Module.h"

class SysModPrintServer:public Module {

public:

  SysModPrintServer() :Module("PrintServer") {}; //constructor

  //setup Serial
  void setup() {
    Module::setup();
    print("%s Setup:", name);

    Serial.begin(115200);
    delay(5000); //if (!Serial) doesn't seem to work, check with SoftHack007

    print(" %s\n", success?"success":"failed");
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

};

static SysModPrintServer *print;