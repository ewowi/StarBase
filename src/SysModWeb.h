#pragma once //as also included in ModModel
#include "Module.h"

#include "html_ui.h"

#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"

//https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/
//https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/

// Create AsyncWebServer object on port 80
static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static const char *(*processWSFunc)(JsonVariant &) = nullptr;
static StaticJsonDocument<2048> responseDoc;

class SysModWeb:public Module {

public:

  bool modelUpdated = false;

  SysModWeb() :Module("Web") {};

  //setup wifi an async webserver
  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
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
    if(type == WS_EVT_CONNECT) {
      print->println(F("WS client connected."));
      sendDataWs(client, true); //send definition to client
    } else if(type == WS_EVT_DISCONNECT) {
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

          if (processWSFunc) {
            DeserializationError error = deserializeJson(responseDoc, data, len); //data to responseDoc
            if (error) {
              print->print("deserializeJson() of definition failed with code %s\n", error.c_str());
            } else {
              JsonVariant json = responseDoc.as<JsonVariant>();
              const char *error = processWSFunc(json); //processJson, adds to responsedoc

              if (responseDoc.size()) {
                // char resStr[200]; 
                // serializeJson(responseDoc, resStr, 200);
                // print->print("WS_EVT_DATA send response %s\n", resStr);

                //uiFun only send to requesting client
                if (responseDoc["uiFun"].isNull())
                  sendDataWs(responseDoc.as<JsonVariant>());
                else
                  sendDataWs(client, responseDoc.as<JsonVariant>());
              }
            }
          }
          else print->print("WS_EVT_DATA no processWSFunc\n");
        }
      }
    } else if(type == WS_EVT_ERROR){
      //error was received from the other end
      print->print("WS error. %s\n", client->remoteIP().toString().c_str());
    } else if(type == WS_EVT_PONG){
      //pong message was received (in response to a ping request maybe)
      print->print("WS pong. %s\n", client->remoteIP().toString().c_str());
    }
  }

  //send json to client or all clients
  static void sendDataWs(AsyncWebSocketClient * client = nullptr, JsonVariant json = JsonVariant()) {
    if (ws.count()) {
      AsyncWebSocketMessageBuffer * wsBuf;

      size_t len = measureJson(json);
      wsBuf = ws.makeBuffer(len); // will not allocate correct memory sometimes on ESP8266

      wsBuf->lock();
      if (wsBuf) {
        serializeJson(json, (char *)wsBuf->get(), len);
        if (client) {
          client->text(wsBuf);
          // DEBUG_PRINTLN(F("to a single client."));
        } else {
          ws.textAll(wsBuf);
          // DEBUG_PRINTLN(F("to multiple clients."));
        }
      }
   
      wsBuf->unlock();
      ws._cleanBuffers();
    }
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
    // File f = files->open(path, "r");
    // if (!f) {
    //   print->print("addURL error opening file %s", path);
    //   return false;
    // } else {
      // print->print("addURL File %s size %d\n", path, f.size());

    server.on(uri, HTTP_GET, [uri, path, contentType](AsyncWebServerRequest *request) {
      print->print("Webserver: client request %s %s %s", uri, path, contentType);

      if (strcmp(path, "/index.htm") == 0) {
        AsyncWebServerResponse *response;
        response = request->beginResponse_P(200, "text/html", PAGE_index, PAGE_index_L);

        response->addHeader(FPSTR("Content-Encoding"),"gzip");
        // setStaticContentCacheHeaders(response);
        request->send(response);
      } else {
        request->send(LittleFS, path, contentType);
      }

      print->print("!\n");
    });
    // }
    // f.close();
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
  //curl -X POST "http://4.3.2.1/json" -d '{"bri":20}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"fx":2}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.121.196/json" -d '{"nrOfLeds":2000}' -H "Content-Type: application/json"

  bool setupJsonHandlers(const char * uri, const char *(*processFunc)(JsonVariant &)) {
    processWSFunc = processFunc; //for WebSocket requests

    //URL handler
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/json", [processFunc](AsyncWebServerRequest *request, JsonVariant &json) {
      responseDoc.clear();
      char resStr[200]; 
      serializeJson(json, resStr, 200);
      print->print("AsyncCallbackJsonWebHandler json %s\n", resStr);
      const char * pErr = processFunc(json); //processJson
      if (responseDoc.size()) {
        serializeJson(responseDoc, resStr, 200);
        print->print("processJsonUrl response %s\n", resStr);
        request->send(200, "text/plain", resStr);
      }
      else
        request->send(200, "text/plain", "OKOK");

    });
    server.addHandler(handler);
    return true;
  }

  void addResponse(JsonObject object, const char * key, const char * value) {
    const char * id = object["id"];
    if (responseDoc[id].isNull()) responseDoc.createNestedObject(id);
    responseDoc[id][key] = value;
  }

  void addResponseV(JsonObject object, const char * key, const char * format, ...) {
    const char * id = object["id"];
    if (responseDoc[id].isNull()) responseDoc.createNestedObject(id);
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);

    va_end(args);

    responseDoc[id][key] = value;
  }

  void addResponseInt(JsonObject object, const char * key, int value) { //temporaty, use overloading
    const char * id = object["id"];
    if (responseDoc[id].isNull()) responseDoc.createNestedObject(id);
    responseDoc[id][key] = value;
  }
  void addResponseBool(JsonObject object, const char * key, bool value) { //temporaty, use overloading
    const char * id = object["id"];
    if (responseDoc[id].isNull()) responseDoc.createNestedObject(id);
    responseDoc[id][key] = value;
  }
  JsonArray addResponseArray(JsonObject object, const char * key) {
    const char * id = object["id"];
    if (responseDoc[id].isNull()) responseDoc.createNestedObject(id);
    return responseDoc[id].createNestedArray(key);
  }

};

static SysModWeb *web;