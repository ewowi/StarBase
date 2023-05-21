#include "module.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

//https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/
//https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

class ModuleWebServer:public Module {

public:

  const char* ssid = "WifiSSID";
  const char* pass = "WifiPass";

  ModuleWebServer() :Module("WebServer") {}; //constructor

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    WiFi.begin(ssid, pass);
  
    print->print(" Connecting to WiFi %s / ", ssid);
    for (int i = 0; i < strlen(pass); i++) print->print("*");
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      print->print(".");
    }
    print->print("!");
  
    print->print(" IP %s", WiFi.localIP().toString());
  
    server.begin();

    print->print(" %s\n", success?"success":"failed");
  }

  //add an url to the webserver to listen to
  bool addURL(const char * uri, const String& path, const String& contentType) {
    File file = LittleFS.open(path, FILE_READ);
    if (!file) {
      print->print(" AddURL There was an error opening the file %s", path);
      return false;
    } else {
      print->print(" File %s size %d", path, file.size());

      server.on(uri, HTTP_GET, [uri, path, contentType](AsyncWebServerRequest *request) {
        print->print("Webserver: client request %s %s %s", uri, path, contentType);
        request->send(LittleFS, path, contentType);
        print->print("!\n");
      });
    }
    file.close();
    return true;
  }

  void loop(){
    // Module::loop();
  }

};

ModuleWebServer *web;