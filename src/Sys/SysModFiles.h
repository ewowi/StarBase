/*
   @title     StarMod
   @file      SysModFiles.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "LittleFS.h"

struct FileDetails {
  char name[32];
  size_t size;
};

class SysModFiles: public SysModule {

public:

  std::vector<FileDetails> fileList;

  SysModFiles();
  void setup();
  void loop();
  void loop10s();

  bool remove(const char * path);

  size_t usedBytes();

  size_t totalBytes();

  File open(const char * path, const char * mode, const bool create = false);

  void filesChange();

  //get the file names and size in an array
  void dirToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //get back the name of a file based on the sequence
  bool seqNrToName(char * fileName, size_t seqNr);

  //reads file and load it in json
  //name is copied from WLED but better to call it readJsonFrom file
  bool readObjectFromFile(const char* path, JsonDocument* dest);

  //write json into file
  //name is copied from WLED but better to call it readJsonFrom file
  //candidate for deletion as taken over by JsonRDWS
  // bool writeObjectToFile(const char* path, JsonDocument* dest);

  //remove files meeting filter condition, if no filter, all, if reverse then all but filter
  void removeFiles(const char * filter = nullptr, bool reverse = false);

  bool readFile(const char * path);

private:
  static bool filesChanged;

};

static SysModFiles *files;