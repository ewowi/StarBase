#pragma once
#include "module.h"
#include "LittleFS.h"

class SysModFiles:public Module {

public:

  SysModFiles();
  void setup();
  void loop();

  bool remove(const char * path);

  size_t usedBytes();

  size_t totalBytes();

  File open(const char * path, const char * mode, const bool create = false);

  static void dirToJson(JsonArray array);

};

static SysModFiles *files;