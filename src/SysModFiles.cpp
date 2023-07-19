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
  parentObject = ui->initGroup(parentObject, name);

  JsonObject fileListObject = ui->initMany(parentObject, "flist", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object, "label", "Files");
    web->addResponse(object, "comment", "List of files");
    JsonArray rows = web->addResponseArray(object, "many");
    dirToJson(rows);
  });
  ui->initDisplay(fileListObject, "flName", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object, "label", "Name");
  });
  ui->initDisplay(fileListObject, "flSize", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object, "label", "Size (B)");
  });

  ui->initDisplay(parentObject, "drsize", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object, "label", "Total FS size");
  });

  web->addUpload("/upload");

}

void SysModFiles::loop(){
  // Module::loop();

  if (millis() - secondMillis >= 1000 || !secondMillis) {
    secondMillis = millis();

    mdl->setValueV("drsize", "%d / %d B", usedBytes(), totalBytes());

        // if something changed in clist
    if (filesChanged) {
      filesChanged = false;

      ui->processUiFun("flist");

    }
  }
}

bool SysModFiles::remove(const char * path) {
  filesChanged = true;
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
void SysModFiles::dirToJson(JsonArray array) {
  File root = LittleFS.open("/");

  File file = root.openNextFile();

  while(file){

    JsonArray row = array.createNestedArray();
    row.add((char *)file.name());  //create a copy!
    row.add(file.size());

    Serial.printf("FILE: %s %d\n", file.name(), file.size());

    file = root.openNextFile();
  }
}

void SysModFiles::dirToJson2(JsonArray array) {
  File root = LittleFS.open("/");

  File file = root.openNextFile();

  while(file){

    array.add((char *)file.name());

    Serial.printf("FILE: %s %d\n", file.name(), file.size());

    file = root.openNextFile();
  }
}

void SysModFiles::filesChange() {
  filesChanged = true;
}