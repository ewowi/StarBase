#include "module.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "ArduinoJson.h"

//https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

StaticJsonDocument<10240> doc;

class ModuleWebServer:public Module {

public:

  const char* ssid = "ssid";
  const char* pass = "pass";

  unsigned long lastInterfaceUpdate = 0;

  ModuleWebServer() :Module("WebServer") {}; //constructor

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    print->print(" Connecting to WiFi %s / ", ssid);
    WiFi.begin(ssid, pass);
    for (int i = 0; i < strlen(pass); i++) print->print("*");
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      print->print(".");
    }
    print->print("!");
  
    print->print(" IP %s", WiFi.localIP().toString().c_str());
  
    ws.onEvent(wsEvent);
    server.addHandler(&ws);

    server.begin();

    print->print(" %s\n", success?"success":"failed");
  }

  void loop() {
    // Module::loop();
    if (millis() - lastInterfaceUpdate > 2000) {
      sendDataWs(); //send new data
      lastInterfaceUpdate = millis();
    }
  }

  static void wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

    if(type == WS_EVT_CONNECT){
  
      Serial.println("Websocket client connection received");
      sendDataWs(client, true);
  
    } else if(type == WS_EVT_DISCONNECT){
      Serial.println("Client disconnected");
    }
  }

  static void sendDataWs(AsyncWebSocketClient * client = nullptr, bool inclDef = false)
  {
    if (!ws.count()) return;
    AsyncWebSocketMessageBuffer * buffer;

    doc["inclDef"] = inclDef;

    size_t len = measureJson(doc);
    buffer = ws.makeBuffer(len); // will not allocate correct memory sometimes on ESP8266

    buffer->lock();
    
    serializeJson(doc, (char *)buffer->get(), len);
    if (client) {
      client->text(buffer);
      // DEBUG_PRINTLN(F("to a single client."));
    } else {
      ws.textAll(buffer);
      // DEBUG_PRINTLN(F("to multiple clients."));
    }
    buffer->unlock();
    ws._cleanBuffers();
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

  bool processURL(const char * uri, void (*func)(AsyncWebServerRequest *)) {
    server.on(uri, HTTP_GET, [uri, func](AsyncWebServerRequest *request) {
      func(request);
    });
    return true;
  }

};

static ModuleWebServer *web;