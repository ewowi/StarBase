/*
   @title     StarMod
   @file      SysModFiles.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModFiles.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModModel.h"

// #include <FS.h>

bool SysModFiles::filesChanged = false;

SysModFiles::SysModFiles() :SysModule("Files") {
  if (!LittleFS.begin(true)) { //true: formatOnFail
    USER_PRINTF(" An Error has occurred while mounting File system");
    USER_PRINTF(" fail\n");
    success = false;
  }
};

//setup filesystem
void SysModFiles::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name);
  if (parentVar["o"] > -1000) parentVar["o"] = -2000; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

  JsonObject tableVar = ui->initTable(parentVar, "fileTbl", nullptr, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
    {
      web->addResponse(var["id"], "label", "Files");
      web->addResponse(var["id"], "comment", "List of files");
      JsonArray rows = web->addResponseA(var["id"], "value");  //overwrite the value
      dirToJson(rows);
      return true;
    }
    case f_AddRow: {
      USER_PRINTF("chFun addRow %s[%d] = %s\n", var["id"].as<const char *>(), rowNr, var["value"].as<String>().c_str());
      web->getResponseObject()["addRow"]["rowNr"] = rowNr;
      //add a row with all defaults
      return true;
    }
    case f_DelRow: {
      USER_PRINTF("chFun delRow %s[%d] = %s\n", var["id"].as<const char *>(), rowNr, var["value"].as<String>().c_str());
      if (rowNr != UINT8_MAX) {
        // call uiFun of tbl to fill responseObject with files
        ui->varFunctions[var["fun"]](var, UINT8_MAX, f_UIFun);
        //trick to get the table values tbd: use column values
        JsonObject responseObject = web->getResponseObject();
        JsonArray row = responseObject["fileTbl"]["value"][rowNr];
        if (!row.isNull()) {
          const char * fileName = row[0]; //first column
          print->printJson("delete file", row);
          this->removeFiles(fileName, false);
        }
      }
      print->printJson(" ", var);
      return true;
    }
    default: return false;
  }});

  ui->initText(tableVar, "flName", nullptr, UINT8_MAX, 32, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      web->addResponse(var["id"], "label", "Name");
      return true;
    default: return false;
  }});

  ui->initNumber(tableVar, "flSize", UINT16_MAX, 0, UINT16_MAX, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      web->addResponse(var["id"], "label", "Size (B)");
      return true;
    default: return false;
  }});

  ui->initURL(tableVar, "flLink", nullptr, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      web->addResponse(var["id"], "label", "Show");
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "drsize", nullptr, UINT8_MAX, 32, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      web->addResponseV(var["id"], "value", "%d / %d B", files->usedBytes(), files->totalBytes());
      web->addResponse(var["id"], "label", "Total FS size");
      return true;
    default: return false;
  }});
}

void SysModFiles::loop(){
  // SysModule::loop();

  if (filesChanged) {
    mdl->setUIValueV("drsize", "%d / %d B", usedBytes(), totalBytes());
    filesChanged = false;

    ui->processUiFun("fileTbl");
  }
}

bool SysModFiles::remove(const char * path) {
  filesChange();
  USER_PRINTF("File remove %s\n", path);
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
        array.add(JsonString(file.name(), JsonString::Copied));
      }
      else {
        JsonArray row = array.createNestedArray();
        row.add(JsonString(file.name(), JsonString::Copied));
        row.add(file.size());
        char urlString[32] = "file/";
        strncat(urlString, file.name(), sizeof(urlString)-1);
        row.add(JsonString(urlString, JsonString::Copied));
      }
      // USER_PRINTF("FILE: %s %d\n", file.name(), file.size());
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
      USER_PRINTF("seqNrToName: %s %d\n", file.name(), file.size());
      root.close();
      strncat(fileName, "/", 31); //add root prefix, fileName is 32 bytes but sizeof doesn't know so cheating
      strncat(fileName, file.name(), 31);
      file.close();
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
    USER_PRINTF("File %s open not successful\n", path);
    return false;
  }
  else { 
    USER_PRINTF(PSTR("File %s open to read, size %d bytes\n"), path, (int)f.size());
    DeserializationError error = deserializeJson(*dest, f);
    if (error) {
      print->printJDocInfo("readObjectFromFile", *dest);
      USER_PRINTF("readObjectFromFile deserializeJson failed with code %s\n", error.c_str());
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
//     print->println("  success");
//     serializeJson(*dest, f);
//     f.close();
//     filesChange();
//     return true;
//   } else {
//     print->println("  fail");
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
