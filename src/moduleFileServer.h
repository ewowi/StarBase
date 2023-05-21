
#include "module.h"

#include <WiFi.h>
#include "LittleFS.h"

//https://randomnerdtutorials.com/esp32-vs-code-platformio-spiffs/
//https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm

class ModuleFileServer:public Module {

public:

  ModuleFileServer() :Module("FileServer") {}; //constructor

  //setup filesystem
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    if (!LittleFS.begin()) {
      print->print(" An Error has occurred while mounting File system");
      print->print(" fail\n");
      success = false;
    }

    print->print(" %s\n", success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

};

ModuleFileServer *file;