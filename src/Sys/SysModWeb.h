/*
   @title     StarMod
   @file      SysModWeb.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "SysModPrint.h"

#include <ESPAsyncWebServer.h>

class SysModWeb:public SysModule {

public:
  static AsyncWebSocket *ws;
  static SemaphoreHandle_t wsMutex;

  SysModWeb();

  void setup();
  void loop();
  void loop1s();

  void reboot();

  void connectedChanged();

  static void wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
  
  //send json to client or all clients
  static void sendDataWs(JsonVariant json = JsonVariant(), AsyncWebSocketClient * client = nullptr);
  static void sendDataWs(std::function<void(AsyncWebSocketMessageBuffer *)> fill, size_t len, bool isBinary, AsyncWebSocketClient * client = nullptr);

  //add an url to the webserver to listen to
  bool addURL(const char * uri, const char * contentType, const char * path = nullptr, const uint8_t * content = nullptr, size_t len = -1);

  //Is this an IP?
  bool isIp(String str) {
    for (size_t i = 0; i < str.length(); i++) {
      int c = str.charAt(i);
      if (c != '.' && (c < '0' || c > '9')) {
        return false;
      }
    }
    return true;
  }

  bool captivePortal(AsyncWebServerRequest *request)
  {
    if (ON_STA_FILTER(request)) return false; //only serve captive in AP mode
    String hostH;
    if (!request->hasHeader("Host")) return false;
    hostH = request->getHeader("Host")->value();

    if (!isIp(hostH)) {// && hostH.indexOf("wled.me") < 0 && hostH.indexOf(cmDNS) < 0) {
      USER_PRINTF("Captive portal\n");
      AsyncWebServerResponse *response = request->beginResponse(302);
      response->addHeader(F("Location"), F("http://4.3.2.1"));
      request->send(response);
      return true;
    }
    return false;
  }

  //not used at the moment
  bool processURL(const char * uri, void (*func)(AsyncWebServerRequest *));

  // curl -F 'data=@fixture1.json' 192.168.8.213/upload
  bool addUpload(const char * uri);
  // curl -s -F "update=@/Users/ewoudwijma/Developer/GitHub/ewowi/StarMod/.pio/build/esp32dev/firmware.bin" 192.168.8.102/update /dev/null &
  bool addUpdate(const char * uri);
  bool addFileServer(const char * uri);

  //processJsonUrl handles requests send in javascript using fetch and from a browser or curl
  //try this !!!: curl -X POST "http://192.168.121.196/json" -d '{"pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"bri":20}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.125/json" -d '{"fx":2}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.152/json" -d '{"nrOfLeds":2000}' -H "Content-Type: application/json"

  //set ws var and create AsyncCallbackJsonWebHandler , currently for /json
  bool setupJsonHandlers(const char * uri, const char * (*processFunc)(JsonVariant &));

  template <typename Type>
  void addResponse(const char * id, const char * key, Type value) {
    JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
    // if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
    responseVariant[id][key] = value;
  }

  JsonArray addResponseA(const char * id, const char * key) {
    JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
    // if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
    return responseVariant[id].createNestedArray(key);
  }

  void addResponseV(const char * id, const char * key, const char * format, ...) {
    JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
    // if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);

    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    responseVariant[id][key] = JsonString(value, JsonString::Copied);
  }

  void clientsToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //gets the right responseDoc, depending on which task you are in, alternative for requestJSONBufferLock
  JsonDocument * getResponseDoc();

  //Currently only WLED style state and info
  static void serveJson(AsyncWebServerRequest *request);

private:
  bool modelUpdated = false;

  static bool clientsChanged;

  static AsyncWebServer *server;
  static const char * (*processWSFunc)(JsonVariant &);

  static DynamicJsonDocument *responseDocLoopTask;
  static DynamicJsonDocument *responseDocAsyncTCP;

  static unsigned long sendDataWsCounter;

};

static SysModWeb *web;