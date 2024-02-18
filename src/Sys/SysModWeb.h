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

#ifdef STARMOD_USE_Psychic
  #include <PsychicHttp.h>
#else
  #include <ESPAsyncWebServer.h>
#endif


#ifdef STARMOD_USE_Psychic
  #define WebRequest PsychicRequest
  #define WebClient PsychicWebSocketClient
  #define WebServer PsychicHttpServer
  #define WebSocket PsychicWebSocketHandler 
  #define WebResponse PsychicResponse
#else
  #define WebRequest AsyncWebServerRequest
  #define WebClient AsyncWebSocketClient
  #define WebServer AsyncWebServer
  #define WebSocket AsyncWebSocket 
  #define WebResponse AsyncWebServerResponse
#endif

class SysModWeb:public SysModule {

public:
  static WebSocket *ws;
  static SemaphoreHandle_t wsMutex;

  SysModWeb();

  void setup();
  void loop();
  void loop1s();

  void reboot();

  void connectedChanged();

  static void wsEvent(WebSocket * ws, WebClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
  
  //send json to client or all clients
  static void sendDataWs(JsonVariant json = JsonVariant(), WebClient * client = nullptr);
  static void sendDataWs(std::function<void(AsyncWebSocketMessageBuffer *)> fill, size_t len, bool isBinary, WebClient * client = nullptr);

  //add an url to the webserver to listen to
  static void serveIndex(WebRequest *request);
  //mdl and WLED style state and info
  static void serveJson(WebRequest *request);


  // curl -F 'data=@fixture1.json' 192.168.8.213/upload
  static void serveUpload(WebRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
  // curl -s -F "update=@/Users/ewoudwijma/Developer/GitHub/ewowi/StarMod/.pio/build/esp32dev/firmware.bin" 192.168.8.102/update /dev/null &
  static void serveUpdate(WebRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
  static void serveFiles(WebRequest *request);

  //processJsonUrl handles requests send in javascript using fetch and from a browser or curl
  //try this !!!: curl -X POST "http://192.168.121.196/json" -d '{"pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"bri":20}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.125/json" -d '{"fx":2}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.152/json" -d '{"nrOfLeds":2000}' -H "Content-Type: application/json"

  //handle "v" and processJson (on /json)
  static void jsonHandler(WebRequest *request, JsonVariant json);

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

  bool captivePortal(WebRequest *request)
  {
    if (ON_STA_FILTER(request)) return false; //only serve captive in AP mode
    String hostH;
    if (!request->hasHeader("Host")) return false;
    hostH = request->getHeader("Host")->value();

    if (!isIp(hostH)) {// && hostH.indexOf("wled.me") < 0 && hostH.indexOf(cmDNS) < 0) {
      USER_PRINTF("Captive portal\n");
      WebResponse *response = request->beginResponse(302);
      response->addHeader(F("Location"), F("http://4.3.2.1"));
      request->send(response);
      return true;
    }
    return false;
  }

  template <typename Type>
  void addResponse(const char * id, const char * key, Type value, uint8_t rowNr = UINT8_MAX) {
    JsonObject responseObject = getResponseObject();
    // if (responseObject[id].isNull()) responseObject[id].to<JsonObject>();;
    if (rowNr == UINT8_MAX)
      responseObject[id][key] = value;
    else {
      if (!responseObject[id][key].is<JsonArray>())
        responseObject[id][key].to<JsonArray>();
      responseObject[id][key][rowNr] = value;
    }
  }

  JsonArray addResponseA(const char * id, const char * key) {
    JsonObject responseObject = getResponseObject();
    // if (responseObject[id].isNull()) responseObject[id].to<JsonObject>();;
    return responseObject[id][key].to<JsonArray>();
  }

  void addResponseV(const char * id, const char * key, const char * format, ...) {
    JsonObject responseObject = getResponseObject();
    // if (responseObject[id].isNull()) responseObject[id].to<JsonObject>();;

    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    responseObject[id][key] = JsonString(value, JsonString::Copied);
  }

  void clientsToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //gets the right responseDoc, depending on which task you are in, alternative for requestJSONBufferLock
  JsonDocument * getResponseDoc();
  JsonObject getResponseObject();
  void sendResponseObject(WebClient * client = nullptr);

  static void printClient(const char * text, WebClient * client) {
    USER_PRINTF("%s client: %d ...%d q:%d l:%d s:%d (#:%d)\n", text, client?client->id():-1, client?client->remoteIP()[3]:-1, client->queueIsFull(), client->queueLength(), client->status(), client->server()->count());
    //status: { WS_DISCONNECTED, WS_CONNECTED, WS_DISCONNECTING }
  }

private:
  bool modelUpdated = false;

  static bool clientsChanged;

  static WebServer *server;

  static JsonDocument *responseDocLoopTask;
  static JsonDocument *responseDocAsyncTCP;

  static unsigned long sendDataWsCounter;
};

static SysModWeb *web;