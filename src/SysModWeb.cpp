#include "SysModWeb.h"
#include "SysModModel.h"
#include "SysModUI.h"

  AsyncWebServer * SysModWeb::server = nullptr;
  AsyncWebSocket * SysModWeb::ws = nullptr;
  const char * (*SysModWeb::processWSFunc)(JsonVariant &) = nullptr;
  DynamicJsonDocument * SysModWeb::responseDoc0 = nullptr;
  DynamicJsonDocument * SysModWeb::responseDoc1 = nullptr;
  bool SysModWeb::clientsChanged = false;

  void SysModWeb::setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void SysModWeb::sendDataWs(AsyncWebSocketClient * client, JsonVariant json) {
    if (!ws) {
      print->print("no ws\n");
      return;
    }
    ws->cleanupClients();

    if (ws->count()) {
      size_t len = measureJson(json);
      AsyncWebSocketMessageBuffer *wsBuf = ws->makeBuffer(len); // will not allocate correct memory sometimes on ESP8266

      if (wsBuf) {
        wsBuf->lock();
        serializeJson(json, (char *)wsBuf->get(), len);
        if (client) {
          if (!client->queueIsFull() && client->status() == WS_CONNECTED) 
            client->text(wsBuf);
          else 
            printClient("sendDataWs client full or not connected", client);

          // DEBUG_PRINTLN(F("to a single client."));
        } else {
          for (auto client:ws->getClients()) {
            if (!client->queueIsFull() && client->status() == WS_CONNECTED) 
              client->text(wsBuf);
          else 
            printClient("sendDataWs client full or not connected", client);
          }
          // DEBUG_PRINTLN(F("to multiple clients."));
        }
        wsBuf->unlock();
        ws->_cleanBuffers();
      }
      else {
        print->print("sendDataWs WS buffer allocation failed\n");
        ws->closeAll(1013); //code 1013 = temporary overload, try again later
        ws->cleanupClients(0); //disconnect all clients to release memory
        ws->_cleanBuffers();
      }

    }
  }

  //specific json data send to all clients
  void SysModWeb::sendDataWs(JsonVariant json) {
    sendDataWs(nullptr, json);
  }

  void SysModWeb::sendDataWs(AsyncWebSocketClient * client, bool inclDef) {
    mdl->model[0]["incldef"] = inclDef;
    sendDataWs(client, *mdl->model);
  }

  void SysModWeb::addResponse(JsonObject object, const char * key, const char * value) {
    const char * id = object["id"];
    JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?responseDoc0:responseDoc1)->as<JsonVariant>();
    if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
    responseVariant[id][key] = value;
  }

  void SysModWeb::addResponseV(JsonObject object, const char * key, const char * format, ...) {
    const char * id = object["id"];
    JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?responseDoc0:responseDoc1)->as<JsonVariant>();
    if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);

    va_end(args);

    responseVariant[id][key] = value;
  }

  void SysModWeb::addResponseInt(JsonObject object, const char * key, int value) { //temporary, use overloading
    const char * id = object["id"];
    JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?responseDoc0:responseDoc1)->as<JsonVariant>();
    if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
    responseVariant[id][key] = value;
  }
  void SysModWeb::addResponseBool(JsonObject object, const char * key, bool value) { //temporary, use overloading
    const char * id = object["id"];
    JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?responseDoc0:responseDoc1)->as<JsonVariant>();
    if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
    responseVariant[id][key] = value;
  }
  JsonArray SysModWeb::addResponseArray(JsonObject object, const char * key) {
    const char * id = object["id"];
    JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?responseDoc0:responseDoc1)->as<JsonVariant>();
    if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
    return responseVariant[id].createNestedArray(key);
  }
