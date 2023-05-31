#include "Module.h"

#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"

//https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/
//https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

StaticJsonDocument<10240> model;

class SysModWebServer:public Module {

public:

  bool modelUpdated = false;

  SysModWebServer() :Module("WebServer") {}; //constructor

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    JsonArray root = model.to<JsonArray>(); //create

    print->print(" %s\n", success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    if (modelUpdated) {
      sendDataWs(nullptr, false); //send new data
      modelUpdated = false;
    }
  }

  void connected() {
      ws.onEvent(wsEvent);
      server.addHandler(&ws);

      server.begin();

      print->print("%s server (re)started\n", name);
  }

  static void wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){
      print->println(F("WS client connected."));
      sendDataWs(client, true);
    } else if(type == WS_EVT_DISCONNECT){
      print->print("WS Client disconnected\n");
    } else if(type == WS_EVT_DATA){
      print->println(F("WS event data."));
    } else if(type == WS_EVT_ERROR){
      //error was received from the other end
      print->println(F("WS error."));
    } else if(type == WS_EVT_PONG){
      //pong message was received (in response to a ping request maybe)
      print->println(F("WS pong."));
    }
  }

  //send json to client or all clients
  static void sendDataWs(AsyncWebSocketClient * client = nullptr, JsonVariant json = JsonVariant()) {
    if (!ws.count()) return;

    AsyncWebSocketMessageBuffer * buffer;

    size_t len = measureJson(json);
    buffer = ws.makeBuffer(len); // will not allocate correct memory sometimes on ESP8266

    buffer->lock();
   
    serializeJson(json, (char *)buffer->get(), len);
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

  //specific json data send to all clients
  static void sendDataWs(JsonVariant json) {
    sendDataWs(nullptr, json);
  }

  //complete document send to client or all clients
  static void sendDataWs(AsyncWebSocketClient * client = nullptr, bool inclDef = false) {
    model[0]["incldef"] = inclDef;
    sendDataWs(client, model);
  }

  //add an url to the webserver to listen to
  bool addURL(const char * uri, const String& path, const String& contentType) {
    File file = LittleFS.open(path, FILE_READ);
    if (!file) {
      print->print("addURL error opening file %s", path);
      return false;
    } else {
      print->print("addURL File %s size %d\n", path, file.size());

      server.on(uri, HTTP_GET, [uri, path, contentType](AsyncWebServerRequest *request) {
        print->print("Webserver: client request %s %s %s", uri, path, contentType);
        request->send(LittleFS, path, contentType);
        print->print("!\n");
      });
    }
    file.close();
    return true;
  }

  //not used at the moment
  bool processURL(const char * uri, void (*func)(AsyncWebServerRequest *)) {
    server.on(uri, HTTP_GET, [uri, func](AsyncWebServerRequest *request) {
      func(request);
    });
    return true;
  }

  bool processJSONURL(const char * uri, void (*func)(JsonVariant &)) {
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/json", [func](AsyncWebServerRequest *request, JsonVariant &json) {
      func(json);
      request->send(200, "text/plain", "OK");
    });
    server.addHandler(handler);
    return true;
  }

};

static SysModWebServer *web;