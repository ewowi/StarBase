#pragma once
#include "Module.h"

#include <ESPAsyncWebServer.h>

class SysModWeb:public Module {

public:
// TODO: which of these fields should be private?
  static AsyncWebSocket *ws;

  SysModWeb();

  void setup();

  void loop();

  void connected2();

  static void wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

  static void printClient(const char * text, AsyncWebSocketClient * client);

  //send json to client or all clients
  static void sendDataWs(AsyncWebSocketClient * client = nullptr, JsonVariant json = JsonVariant());

  //specific json data send to all clients
  static void sendDataWs(JsonVariant json);

  //complete document send to client or all clients
  static void sendDataWs(AsyncWebSocketClient * client = nullptr, bool inclDef = false);

  //add an url to the webserver to listen to
  bool addURL(const char * uri, const char * path, const char * contentType);

  //not used at the moment
  bool processURL(const char * uri, void (*func)(AsyncWebServerRequest *));

// curl -F 'data=@ledfix1.json' 192.168.8.213/upload
  bool addUpload(const char * uri);
  bool addFileServer(const char * uri);

  //processJsonUrl handles requests send in javascript using fetch and from a browser or curl
  //try this !!!: curl -X POST "http://192.168.121.196/json" -d '{"Pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"Pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"bri":20}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.152/json" -d '{"fx":2}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.152/json" -d '{"nrOfLeds":2000}' -H "Content-Type: application/json"

  bool setupJsonHandlers(const char * uri, const char * (*processFunc)(JsonVariant &));

  void addResponse(const char * id, const char * key, const char * value);

  void addResponseV(const char * id, const char * key, const char * format, ...);

  void addResponseI(const char * id, const char * key, int value);
  void addResponseB(const char * id, const char * key, bool value);
  JsonArray addResponseA(const char * id, const char * key);

  void clientsToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //gets the right responseDoc, depending on which task you are in
  JsonDocument * getResponseDoc();
  
private:
  bool modelUpdated = false;
  static bool clientsChanged;

  static AsyncWebServer *server;
  static const char * (*processWSFunc)(JsonVariant &);

  static DynamicJsonDocument *responseDoc0;
  static DynamicJsonDocument *responseDoc1;

};

static SysModWeb *web;