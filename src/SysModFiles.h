#pragma once
#include "Module.h"
#include "LittleFS.h"

class SysModFiles:public Module {

public:


  SysModFiles();
  void setup();
  void loop();

  // void handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);

  bool remove(const char * path);

  size_t usedBytes();

  size_t totalBytes();

  File open(const char * path, const char * mode, const bool create = false);

  void filesChange();

  //get the file names and size in an array of arrays
  static void dirToJson(JsonArray array);

  //get the file names in an array
  static void dirToJson2(JsonArray array);

  //get back the name of a file based on the sequence
  static char * seqNrToName(size_t seqNr);

  //reads file and load it in json
  //name is copied from WLED but better to call it readJsonFrom file
  bool readObjectFromFile(const char* path, JsonDocument* dest);

  //write json into file
  //name is copied from WLED but better to call it readJsonFrom file
  bool writeObjectToFile(const char* path, JsonDocument* dest);

private:
  static bool filesChanged;

};

static SysModFiles *files;