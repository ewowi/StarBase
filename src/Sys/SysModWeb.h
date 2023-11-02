/*
   @title     StarMod
   @file      SysModWeb.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include "SysModule.h"

#include <ESPAsyncWebServer.h>

class SysModWeb:public SysModule {

public:
  static AsyncWebSocket *ws;

  SysModWeb();

  void setup();
  void loop();
  void loop1s();

  void connectedChanged();

  static void wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
  static void wsEvent2(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

  //send json to client or all clients
  static void sendDataWs(JsonVariant json = JsonVariant(), AsyncWebSocketClient * client = nullptr);

  //add an url to the webserver to listen to
  bool addURL(const char * uri, const char * contentType, const char * path = nullptr, const uint8_t * content = nullptr, size_t len = -1);

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

  void addResponse(const char * id, const char * key, const char * value);
  void addResponseArray(const char * id, const char * key, JsonArray value);

  void addResponseV(const char * id, const char * key, const char * format, ...);

  void addResponseI(const char * id, const char * key, int value);
  void addResponseB(const char * id, const char * key, bool value);
  JsonArray addResponseA(const char * id, const char * key);

  void clientsToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //gets the right responseDoc, depending on which task you are in, alternative for requestJSONBufferLock
  JsonDocument * getResponseDoc();

  //Currently only WLED style state and info
  static void serveJson(AsyncWebServerRequest *request);
  
  static unsigned long wsSendBytesCounter;
  static unsigned long wsSendJsonCounter;
  static unsigned long wsSendDataWsCounter;
private:
  bool modelUpdated = false;

  static bool clientsChanged;

  static AsyncWebServer *server;
  static const char * (*processWSFunc)(JsonVariant &);

  static DynamicJsonDocument *responseDocLoopTask;
  static DynamicJsonDocument *responseDocAsyncTCP;

};

static SysModWeb *web;