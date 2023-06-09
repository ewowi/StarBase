#pragma once //as also included in ModModel
#include "Module.h"

#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"

//https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/
//https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/

// Create AsyncWebServer object on port 80
static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static JsonVariant(*processWSFunc)(JsonVariant &);

class SysModWebServer:public Module {

public:

  bool modelUpdated = false;

  SysModWebServer() :Module("WebServer") {};

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    //currently not used as each variable is send individually
    if (modelUpdated) {
      sendDataWs(nullptr, false); //send new data, all clients, no def
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
      sendDataWs(client, true); //send definition to client
    } else if(type == WS_EVT_DISCONNECT){
      print->print("WS Client disconnected\n");
    } else if(type == WS_EVT_DATA){
      print->println(F("WS event data."));
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && info->index == 0 && info->len == len){
        // the whole message is in a single frame and we got all of its data (max. 1450 bytes)
        if(info->opcode == WS_TEXT)
        {
          if (len > 0 && len < 10 && data[0] == 'p') {
            // application layer ping/pong heartbeat.
            // client-side socket layer ping packets are unresponded (investigate)
            client->text(F("pong"));
            return;
          }

          DeserializationError error = deserializeJson(responseDoc, data, len);
          serializeJson(responseDoc, Serial);
          JsonVariant json = responseDoc.as<JsonVariant>();
          JsonVariant result = processWSFunc(json); //func?:nullptr; //processJson

          if (result) {
            char resStr[50]; 
            serializeJson(result, resStr);
            print->print("WS_EVT_DATA response %s\n", resStr);
            sendDataWs(result);
          }
        }
      }
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
  bool addURL(const char * uri, const char * path, const char * contentType) {
    File f = files->open(path, "r");
    if (!f) {
      print->print("addURL error opening file %s", path);
      return false;
    } else {
      print->print("addURL File %s size %d\n", path, f.size());

      server.on(uri, HTTP_GET, [uri, path, contentType](AsyncWebServerRequest *request) {
        print->print("Webserver: client request %s %s %s", uri, path, contentType);
        request->send(LittleFS, path, contentType);
        print->print("!\n");
      });
    }
    f.close();
    return true;
  }

  //not used at the moment
  bool processURL(const char * uri, void (*func)(AsyncWebServerRequest *)) {
    server.on(uri, HTTP_GET, [uri, func](AsyncWebServerRequest *request) {
      func(request);
    });
    return true;
  }

  //processJsonUrl handles requests send in javascript using fetch and from a browser or curl
  //try this !!!: curl -X POST "http://192.168.121.196/json" -d '{"Pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"Pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"Brightness":20}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"Effect":2}' -H "Content-Type: application/json"

  bool processJsonUrl(const char * uri, JsonVariant (*processFunc)(JsonVariant &)) {
    processWSFunc = processFunc; //for WebSocket requests
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/json", [processFunc](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonVariant result = processFunc(json);
      if (!result.isNull()) {
        char resStr[50]; 
        serializeJson(result, resStr);
        print->print("processJsonUrl response %s (not implemented yet\n", resStr);
        // request->send(200, "application/json", result);
        // AsyncJsonResponse* response = new AsyncJsonResponse(&responseDoc, true);  // array document
        // JsonArray lDoc = response->getRoot();
        // lDoc[0] = "een";
        // lDoc[1] = "twee";
        // response->setLength();
        // request->send(response);
        request->send(200, "text/plain", resStr);
      }
      else
        request->send(200, "text/plain", "OKOK");

    });
    server.addHandler(handler);
    return true;
  }

};

static SysModWebServer *web;