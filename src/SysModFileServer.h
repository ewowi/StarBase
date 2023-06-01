// #pragma once
#include "Module.h"

#include "LittleFS.h"

//https://randomnerdtutorials.com/esp32-vs-code-platformio-spiffs/
  // upload under the data folder:
    // Click the PIO icon at the left side bar. The project tasks should open.
    // Select esp32dev (it may be slightly different depending on the board you’re using).
    // Expand the Platform menu.
    // Select Build Filesystem Image.
    // Finally, click Upload Filesystem Image.
//https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm

class SysModFileServer:public Module {

public:

  SysModFileServer() :Module("FileServer") {}; //constructor

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    if (!LittleFS.begin()) {
      print->print(" An Error has occurred while mounting File system");
      print->print(" fail\n");
      success = false;
    }

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

  bool remove(const char * path) {
    return LittleFS.remove(path);
  }

  size_t usedBytes() {
    return LittleFS.usedBytes();
  }

  size_t totalBytes() {
    return LittleFS.totalBytes();
  }
  File open(const char * path, const char * mode, const bool create = false) {
    return LittleFS.open(path, mode, create);
  }

};

static SysModFileServer *file;