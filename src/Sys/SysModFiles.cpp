/*
   @title     StarMod
   @file      SysModFiles.cpp
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModFiles.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModModel.h"

// #include <FS.h>

bool SysModFiles::filesChanged = false;

SysModFiles::SysModFiles() :Module("Files") {
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  if (!LittleFS.begin(true)) { //true: formatOnFail
    print->print(" An Error has occurred while mounting File system");
    print->print(" fail\n");
    success = false;
  }

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

//setup filesystem
void SysModFiles::setup() {
  Module::setup();
  parentVar = ui->initModule(parentVar, name);

  JsonObject tableVar = ui->initTable(parentVar, "fileTbl", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Files");
    web->addResponse(var["id"], "comment", "List of files");
    JsonArray rows = web->addResponseA(var["id"], "table");
    dirToJson(rows);

  });
  ui->initText(tableVar, "flName", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
  });
  ui->initText(tableVar, "flSize", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Size (B)");
  });
  ui->initURL(tableVar, "flLink", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Show");
  });

  ui->initText(parentVar, "drsize", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Total FS size");
  });

  ui->initButton(parentVar, "deleteFiles", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "All but model.json");
  }, [](JsonObject var) {
    print->print("delete files\n");
    files->removeFiles("model.json", true); //all but model.json
  });

  // ui->initURL(parentVar, "urlTest", "file/3DCube202005.json", true);

  web->addUpload("/upload");

  web->addFileServer("/file");

}

void SysModFiles::loop(){
  // Module::loop();

  if (millis() - secondMillis >= 1000) {
    secondMillis = millis();

    mdl->setValueV("drsize", "%d / %d B", usedBytes(), totalBytes());

        // if something changed in fileTbl
    if (filesChanged) {
      filesChanged = false;

      ui->processUiFun("fileTbl");
    }
  }
}

bool SysModFiles::remove(const char * path) {
  filesChange();
  print->print("File remove %s\n", path);
  return LittleFS.remove(path);
}

size_t SysModFiles::usedBytes() {
  return LittleFS.usedBytes();
}

size_t SysModFiles::totalBytes() {
  return LittleFS.totalBytes();
}

File SysModFiles::open(const char * path, const char * mode, const bool create) {
  return LittleFS.open(path, mode, create);
}

// https://techtutorialsx.com/2019/02/24/esp32-arduino-listing-files-in-a-spiffs-file-system-specific-path/
void SysModFiles::dirToJson(JsonArray array, bool nameOnly, const char * filter) {
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {

    if (filter == nullptr || strstr(file.name(), filter) != nullptr) {
      if (nameOnly) {
        array.add((char *)file.name());
      }
      else {
        JsonArray row = array.createNestedArray();
        row.add((char *)file.name());  //create a copy!
        row.add(file.size());
        char urlString[32] = "file/";
        strncat(urlString, file.name(), sizeof(urlString)-1);
        row.add((char *)urlString);  //create a copy!
      }
      // Serial.printf("FILE: %s %d\n", file.name(), file.size());
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}

void SysModFiles::filesChange() {
  filesChanged = true;
}

bool SysModFiles::seqNrToName(char * fileName, size_t seqNr) {

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  size_t counter = 0;
  while (file) {
    if (counter == seqNr) {
      Serial.printf("seqNrToName: %s %d\n", file.name(), file.size());
      root.close();
      strncat(fileName, "/", 31); //add root prefix, fileName is 32 bytes but sizeof doesn't know so cheating
      strncat(fileName, file.name(), 31);
      return true;
    }

    file.close();
    file = root.openNextFile();
    counter++;
  }

  root.close();
  return false;
}

bool SysModFiles::readObjectFromFile(const char* path, JsonDocument* dest) {
  // if (doCloseFile) closeFile();
  File f = open(path, "r");
  if (!f) {
    print->print("File %s open not successful\n", path);
    return false;
  }
  else { 
    print->print(PSTR("File %s open to read, size %d bytes\n"), path, (int)f.size());
    DeserializationError error = deserializeJson(*dest, f);
    if (error) {
      print->printJDocInfo("readObjectFromFile", *dest);
      print->print("readObjectFromFile deserializeJson failed with code %s\n", error.c_str());
      f.close();
      return false;
    } else {
      f.close();
      return true;
    }
  }
}

//candidate for deletion as taken over by JsonRDWS
// bool SysModFiles::writeObjectToFile(const char* path, JsonDocument* dest) {
//   File f = open(path, "w");
//   if (f) {
//     print->println(F("  success"));
//     serializeJson(*dest, f);
//     f.close();
//     filesChange();
//     return true;
//   } else {
//     print->println(F("  fail"));
//     return false;
//   }
// }

void SysModFiles::removeFiles(const char * filter, bool reverse) {
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {
    if (filter == nullptr || reverse?strstr(file.name(), filter) == nullptr: strstr(file.name(), filter) != nullptr) {
      char fileName[32] = "/";
      strncat(fileName, file.name(), sizeof(fileName)-1);
      file.close(); //close otherwise not removeable
      remove(fileName);
    }
    else
      file.close();

    file = root.openNextFile();
  }

  root.close();
}

bool SysModFiles::readFile(const char * path) {
  File f = open(path, "r");
  if (f) {

    while(f.available()) {
      Serial.print((char)f.read());
    }
    Serial.println();

    f.close();
    return true;
  }
  else 
    return false;
}
