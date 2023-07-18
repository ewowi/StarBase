#include "Module.h"
#include "SysModPrint.h"
#include "SysModUI.h"

SysModPrint::SysModPrint() :Module("Print") {
  print("%s %s\n", __PRETTY_FUNCTION__, name);

  Serial.begin(115200);
  delay(5000); //if (!Serial) doesn't seem to work, check with SoftHack007

  print("%s %s %s\n",__PRETTY_FUNCTION__,name, success?"success":"failed");
};

void SysModPrint::setup() {
  Module::setup();

  print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentObject = ui->initGroup(parentObject, name);

  ui->initDisplay(parentObject, "log");

  print("%s %s %s\n",__PRETTY_FUNCTION__,name, success?"success":"failed");
}

void SysModPrint::loop() {
  // Module::loop();
}

size_t SysModPrint::print(const char * format, ...) {
  va_list args;
  va_start(args, format);

  size_t len = vprintf(format, args);

  va_end(args);
  
  // mdl->setValueV("log", "%lu", millis());

  return len;
}

size_t SysModPrint::println(const __FlashStringHelper * x) {
  return Serial.println(x);
}

void SysModPrint::printObject(JsonObject object) {
  char sep[3] = "";
  for (JsonPair pair: object) {
    print("%s%s: %s", sep, pair.key(), pair.value().as<String>().c_str());
    strcpy(sep, ", ");
  }
}

size_t SysModPrint::printJson(const char * text, JsonVariantConst source) {
  Serial.printf("%s ", text);
  size_t size = serializeJson(source, Serial); //for the time being
  Serial.println();
  return size;
}

const char * SysModPrint::fFormat(const char * format, ...) {
  static char msgbuf[32];

  va_list args;
  va_start(args, format);

  size_t len = snprintf(msgbuf, sizeof(msgbuf), format, args);

  va_end(args);

  return msgbuf;
}

