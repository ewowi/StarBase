/*
   @title     StarMod
   @file      SysModPrint.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "Module.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModModel.h"
#include "SysModWeb.h"

SysModPrint::SysModPrint() :Module("Print") {
  print("%s %s\n", __PRETTY_FUNCTION__, name);

  Serial.begin(115200);
  delay(5000); //if (!Serial) doesn't seem to work, check with SoftHack007

  print("%s %s %s\n",__PRETTY_FUNCTION__,name, success?"success":"failed");
};

void SysModPrint::setup() {
  Module::setup();

  print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = ui->initModule(parentVar, name);

  ui->initSelect(parentVar, "pOut", 1, false, [](JsonObject var) { //uiFun default 1 (Serial)
    web->addResponse(var["id"], "label", "Output");
    web->addResponse(var["id"], "comment", "System log to Serial or Net print (WIP)");

    JsonArray rows = web->addResponseA(var["id"], "select");
    rows.add("No");
    rows.add("Serial");
    rows.add("UI");

    web->clientsToJson(rows, true); //ip only
  });

  ui->initTextArea(parentVar, "log", "WIP", true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Show the printed log");
  });

  print("%s %s %s\n",__PRETTY_FUNCTION__,name, success?"success":"failed");
}

void SysModPrint::loop() {
  // Module::loop();
  if (!setupsDone) setupsDone = true;
}

size_t SysModPrint::print(const char * format, ...) {
  va_list args;
  va_start(args, format);

  size_t len = vprintf(format, args);

  va_end(args);
  
  // if (setupsDone) mdl->setValueI("log", (int)millis()/1000);
  //this function looks very sensitive, any chance causes crashes!
  //reason could (very well...) be that setValue also issues print commands...

  return len;
}

size_t SysModPrint::println(const __FlashStringHelper * x) {
  return Serial.println(x);
}

void SysModPrint::printVar(JsonObject var) {
  char sep[3] = "";
  for (JsonPair pair: var) {
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

char * SysModPrint::fFormat(const char * format, ...) {
  static char msgbuf[32];

  va_list args;
  va_start(args, format);

  size_t len = snprintf(msgbuf, sizeof(msgbuf), format, args);

  va_end(args);

  return msgbuf;
}

void SysModPrint::printJDocInfo(const char * text, DynamicJsonDocument source) {
  print("%s  %u / %u (%u%%) (%u %u %u)\n", text, source.memoryUsage(), source.capacity(), 100 * source.memoryUsage() / source.capacity(), source.size(), source.overflowed(), source.nesting());
}

