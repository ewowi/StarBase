/*
   @title     StarMod
   @file      SysModWeb.cpp
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "SysModWeb.h"
#include "SysModModel.h"
#include "SysModUI.h"
#include "SysModPrint.h"
#include "SysModFiles.h"
#include "SysModModules.h"

#include "AsyncJson.h"

#include "html_ui.h"

//https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/
//https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/


AsyncWebServer * SysModWeb::server = nullptr;
AsyncWebSocket * SysModWeb::ws = nullptr;

const char * (*SysModWeb::processWSFunc)(JsonVariant &) = nullptr;

DynamicJsonDocument * SysModWeb::responseDoc0 = nullptr;
DynamicJsonDocument * SysModWeb::responseDoc1 = nullptr;
bool SysModWeb::clientsChanged = false;

unsigned long SysModWeb::wsSendBytesCounter = 0;
unsigned long SysModWeb::wsSendJsonCounter = 0;

SysModWeb::SysModWeb() :Module("Web") {
  ws = new AsyncWebSocket("/ws");
  server = new AsyncWebServer(80);
  responseDoc0 = new DynamicJsonDocument(2048);
  responseDoc1 = new DynamicJsonDocument(2048);
};

void SysModWeb::setup() {
  Module::setup();
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = ui->initModule(parentVar, name);

  JsonObject tableVar = ui->initTable(parentVar, "clist", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Clients");
    web->addResponse(var["id"], "comment", "List of clients");
    JsonArray rows = web->addResponseA(var["id"], "table");
    web->clientsToJson(rows);
  });
  ui->initText(tableVar, "clNr", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Nr");
  });
  ui->initText(tableVar, "clIp", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "IP");
  });
  ui->initCheckBox(tableVar, "clIsFull", false, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Is full");
  }, [](JsonObject var) { //chFun
    print->printJson("clIsFull.chFun", var);
  });
  ui->initSelect(tableVar, "clStatus", -1, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Status");
    //tbd: not working yet in ui
    JsonArray select = web->addResponseA(var["id"], "select");
    select.add("Disconnected"); //0
    select.add("Connected"); //1
    select.add("Disconnecting"); //2
  });

  ui->initText(parentVar, "wsSendBytes");
  ui->initText(parentVar, "wsSendJson");

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModWeb::loop() {
  // Module::loop();

  //currently not used as each variable is send individually
  if (this->modelUpdated) {
    sendDataWs(nullptr, false); //send new data, all clients, no def
    this->modelUpdated = false;
  }

  if (millis() - secondMillis >= 1000) {
    secondMillis = millis();

    // if something changed in clist
    if (clientsChanged) {
      clientsChanged = false;

      ui->processUiFun("clist");
    }

    uint8_t rowNr = 0;
    for (auto client:SysModWeb::ws->getClients()) {
      mdl->setValueB("clIsFull", client->queueIsFull(), rowNr);
      mdl->setValueI("clStatus", client->status());
      rowNr++;
    }

    mdl->setValueV("wsSendBytes", "%lu /s", wsSendBytesCounter);
    wsSendBytesCounter = 0;
    mdl->setValueV("wsSendJson", "%lu /s", wsSendJsonCounter);
    wsSendJsonCounter = 0;
  }
}

void SysModWeb::connectedChanged() {
  if (SysModModules::isConnected) {
    ws->onEvent(wsEvent);
    // ws->onEvent(wsEvent2);
    server->addHandler(ws);

    server->begin();

    // print->print("%s server (re)started\n", name); //causes crash for some reason...
    print->print("server (re)started\n"); //and this not causes crash ??? whats with name?
  }
  //else remove handlers...
}

//WebSocket connection to 'ws://192.168.8.152/ws' failed: The operation couldn’t be completed. Protocol error
//WS error 192.168.8.126 9 (2)
//WS event data 0.0.0.0 (1) 0 0 0=0? 34 0
//WS pong 0.0.0.0 9 (1)
//wsEvent deserializeJson failed with code EmptyInput

// https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/examples/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser.ino
void SysModWeb::wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  // if (!ws->count()) {
  //   print->print("wsEvent no clients\n");
  //   return;
  // }
  if (type == WS_EVT_CONNECT) {
    printClient("WS client connected", client);
    sendDataWs(client, true); //send definition to client
    clientsChanged = true;
  } else if (type == WS_EVT_DISCONNECT) {
    printClient("WS Client disconnected", client);
    clientsChanged = true;
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    // print->print("  info %d %d %d=%d? %d %d\n", info->final, info->index, info->len, len, info->opcode, data[0]);
    if (info->final && info->index == 0 && info->len == len) { //not multipart
      printClient("WS event data", client);
      // the whole message is in a single frame and we got all of its data (max. 1450 bytes)
      if (info->opcode == WS_TEXT)
      {
        if (len > 0 && len < 10 && data[0] == 'p') {
          // application layer ping/pong heartbeat.
          // client-side socket layer ping packets are unresponded (investigate)
          // printClient("WS client pong", client); //crash?
          print->print("pong\n");
          client->text(F("pong"));
          return;
        }

        if (processWSFunc) { //processJson defined
          JsonDocument *responseDoc = web->getResponseDoc();
          responseDoc->clear(); //needed for deserializeJson?
          JsonVariant responseVariant = responseDoc->as<JsonVariant>();

          DeserializationError error = deserializeJson(*responseDoc, data, len); //data to responseDoc

          if (error || responseVariant.isNull()) {
            print->print("wsEvent deserializeJson failed with code %s\n", error.c_str());
            client->text(F("{\"success\":true}")); // we have to send something back otherwise WS connection closes
          } else {
            const char * error = processWSFunc(responseVariant); //processJson, adds to responsedoc

            if (responseVariant.size()) {
              print->printJson("WS_EVT_DATA json", responseVariant);
              print->printJDocInfo("WS_EVT_DATA info", *responseDoc);

              //uiFun only send to requesting client
              if (responseVariant["uiFun"].isNull())
                sendDataWs(responseVariant);
              else
                sendDataWs(client, responseVariant);
            }
            else {
              print->print("WS_EVT_DATA no responseDoc\n");
              client->text(F("{\"success\":true}")); // we have to send something back otherwise WS connection closes
            }
          }
        }
        else {
          print->print("WS_EVT_DATA no processWSFunc\n");
          client->text(F("{\"success\":true}")); // we have to send something back otherwise WS connection closes
        }
        }
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < len; i++) {
          msg += (char) data[i];
        }
      }
      // else {
      //   char buff[3];
      //   for(size_t i=0; i < len; i++) {
      //     sprintf(buff, "%02x ", (uint8_t) data[i]);
      //     msg += buff ;
      //   }
      // }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }

      //message is comprised of multiple frames or the frame is split into multiple packets
      //if(info->index == 0){
        //if (!wsFrameBuffer && len < 4096) wsFrameBuffer = new uint8_t[4096];
      //}

      //if (wsFrameBuffer && len < 4096 && info->index + info->)
      //{

      //}

      // if((info->index + len) == info->len){
      //   if(info->final){
      //     if(info->message_opcode == WS_TEXT) {
      //       client->text(F("{\"error\":9}")); //we do not handle split packets right now
      //       print->print("WS multipart message: we do not handle split packets right now\n");
      //     }
      //   }
      // }
      // print->print("WS multipart message f:%d i:%d len:%d == %d\n", info->final, info->index, info->len, len);
    }
  } else if (type == WS_EVT_ERROR){
    //error was received from the other end
    // printClient("WS error", client); //crashes
    // print->print("WS error\n");
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);

  } else if (type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    // printClient("WS pong", client); //crashes!
    // print->print("WS pong\n");
    // Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  }
}

void SysModWeb::wsEvent2(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    // client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } 
      // else {
      //   char buff[3];
      //   for(size_t i=0; i < info->len; i++) {
      //     sprintf(buff, "%02x ", (uint8_t) data[i]);
      //     msg += buff ;
      //   }
      // }
      Serial.printf("%s\n",msg.c_str());

      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < len; i++) {
          msg += (char) data[i];
        }
      }
      // else {
      //   char buff[3];
      //   for(size_t i=0; i < len; i++) {
      //     sprintf(buff, "%02x ", (uint8_t) data[i]);
      //     msg += buff ;
      //   }
      // }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}


void SysModWeb::printClient(const char * text, AsyncWebSocketClient * client) {
  print->print("%s client: %d %s q:%d s:%d (w:%d)\n", text, client?client->id():-1, client?client->remoteIP().toString().c_str():"No", client->queueIsFull(), client->status(), ws->count());
}

void SysModWeb::sendDataWs(AsyncWebSocketClient * client, JsonVariant json) {
  if (!ws) {
    print->print("sendDataWs no ws\n");
    return;
  }
  // return;
  wsSendJsonCounter++;
  // ws->cleanupClients();

  if (ws->count()) {
    bool okay = false;
    for (auto client:SysModWeb::ws->getClients()) {
      if (client->status() == WS_CONNECTED && !client->queueIsFull()) 
        okay = true;
    }

    if (okay) {

    size_t len = measureJson(json);
    AsyncWebSocketMessageBuffer *wsBuf = ws->makeBuffer(len);

    if (wsBuf) {
      wsBuf->lock();
      serializeJson(json, (char *)wsBuf->get(), len);
      if (client) {
        if (client->status() == WS_CONNECTED && !client->queueIsFull()) 
          client->text(wsBuf);
        else 
          printClient("sendDataWs client full or not connected", client);

        // DEBUG_PRINTLN(F("to a single client."));
      } else {
        for (auto client:ws->getClients()) {
          if (client->status() == WS_CONNECTED && !client->queueIsFull()) 
            client->text(wsBuf);
          else 
            // printClient("sendDataWs client full or not connected", client); //crash??
            print->print("sendDataWs client full or not connected\n");
        }
        // DEBUG_PRINTLN(F("to multiple clients."));
      }
      wsBuf->unlock();
      ws->_cleanBuffers();
    }
    else {
      print->print("sendDataWs WS buffer allocation failed\n");
      ws->closeAll(1013); //code 1013 = temporary overload, try again later
      ws->cleanupClients(); //disconnect all clients to release memory
      ws->_cleanBuffers();
    }
    } //if okay
  }
}

//specific json data send to all clients
void SysModWeb::sendDataWs(JsonVariant json) {
  sendDataWs(nullptr, json);
}

void SysModWeb::sendDataWs(AsyncWebSocketClient * client, bool inclDef) {
  //tbd: remove inclDef paramater (now only used to type overload sendDataWS)
  sendDataWs(client, *SysModModel::model);
}

//add an url to the webserver to listen to
bool SysModWeb::addURL(const char * uri, const char * path, const char * contentType) {
  // File f = files->open(path, "r");
  // if (!f) {
  //   print->print("addURL error opening file %s", path);
  //   return false;
  // } else {
    // print->print("addURL File %s size %d\n", path, f.size());

  server->on(uri, HTTP_GET, [uri, path, contentType](AsyncWebServerRequest *request) {
    print->print("Webserver: client request %s %s %s", uri, path, contentType);

    if (strcmp(path, "/index.htm") == 0) {
      AsyncWebServerResponse *response;
      response = request->beginResponse_P(200, "text/html", PAGE_index, PAGE_index_L);

      response->addHeader(FPSTR("Content-Encoding"),"gzip");
      // setStaticContentCacheHeaders(response);
      request->send(response);
    } 
    // temporary removed!!!
    // else {
    //   request->send(LittleFS, path, contentType);
    // }

    print->print("!\n");
  });
  // }
  // f.close();
  return true;
}

//not used at the moment
bool SysModWeb::processURL(const char * uri, void (*func)(AsyncWebServerRequest *)) {
  server->on(uri, HTTP_GET, [uri, func](AsyncWebServerRequest *request) {
    func(request);
  });
  return true;
}

// curl -F 'data=@ledfix1.json' 192.168.8.213/upload
bool SysModWeb::addUpload(const char * uri) {

  // curl -F 'data=@ledfix1.json' 192.168.8.213/upload
  server->on(uri, HTTP_POST, [](AsyncWebServerRequest *request) {},
  [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                size_t len, bool final) {
    print->print("handleUpload r:%s f:%s i:%d l:%d f:%d\n", request->url().c_str(), filename.c_str(), index, len, final);
    if (!index) {
      String finalname = filename;
      if (finalname.charAt(0) != '/') {
        finalname = '/' + finalname; // prepend slash if missing
      }

      request->_tempFile = files->open(finalname.c_str(), "w");
      // DEBUG_PRINT(F("Uploading "));
      // DEBUG_PRINTLN(finalname);
      // if (finalname.equals("/presets.json")) presetsModifiedTime = toki.second();
    }
    if (len) {
      request->_tempFile.write(data,len);
    }
    if (final) {
      request->_tempFile.close();
      // USER_PRINT(F("File uploaded: "));  // WLEDMM
      // USER_PRINTLN(filename);            // WLEDMM
      // if (filename.equalsIgnoreCase("/cfg.json") || filename.equalsIgnoreCase("cfg.json")) { // WLEDMM
      //   request->send(200, "text/plain", F("Configuration restore successful.\nRebooting..."));
      //   doReboot = true;
      // } else {
      //   if (filename.equals("/presets.json") || filename.equals("presets.json")) {  // WLEDMM
      //     request->send(200, "text/plain", F("Presets File Uploaded!"));
      //   } else
          request->send(200, "text/plain", F("File Uploaded!"));
      // }
      // cacheInvalidate++;
     files->filesChange();
    }
  });
  return true;
}

bool SysModWeb::addFileServer(const char * uri) {

  // AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/json", [](AsyncWebServerRequest *request) {
  // });
  // server->addHandler(handler);

  server->on(uri, HTTP_GET, [uri](AsyncWebServerRequest *request){
    const char * ddd = request->url().c_str();
    const char * path = ddd + strlen(uri); //remove the uri from the path (skip their positions)
    print->print("fileServer request %s %s %s\n", uri, request->url().c_str(), path);
    if(LittleFS.exists(path)) {
      request->send(LittleFS, path, "text/plain");//"application/json");
    }
  });
  return true;
}

bool SysModWeb::setupJsonHandlers(const char * uri, const char * (*processFunc)(JsonVariant &)) {
  processWSFunc = processFunc; //for WebSocket requests

  //URL handler
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/json", [processFunc](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonDocument *responseDoc = web->getResponseDoc();
    responseDoc->clear(); //needed for deserializeJson?
    JsonVariant responseVariant = responseDoc->as<JsonVariant>();

    print->printJson("AsyncCallbackJsonWebHandler", json);
    const char * pErr = processFunc(json); //processJson
    if (responseVariant.size()) {
      char resStr[200]; 
      serializeJson(responseVariant, resStr, 200);
      print->print("processJsonUrl response %s\n", resStr);
      request->send(200, "text/plain", resStr);
    }
    else
      request->send(200, "text/plain", "OKOK");
  });
  server->addHandler(handler);
  return true;
}

void SysModWeb::addResponse(const char * id, const char * key, const char * value) {
  JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
  if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
  responseVariant[id][key] = (char *)value; //copy!!
}

void SysModWeb::addResponseArray(const char * id, const char * key, JsonArray value) {
  JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
  if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
  responseVariant[id][key] = value;
}

void SysModWeb::addResponseV(const char * id, const char * key, const char * format, ...) {
  JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
  if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
  va_list args;
  va_start(args, format);

  // size_t len = vprintf(format, args);
  char value[100];
  vsnprintf(value, sizeof(value), format, args);

  va_end(args);

  responseVariant[id][key] = value;
}

void SysModWeb::addResponseI(const char * id, const char * key, int value) { //temporary, use overloading
  JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
  if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
  responseVariant[id][key] = value;
}
void SysModWeb::addResponseB(const char * id, const char * key, bool value) { //temporary, use overloading
  JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
  if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
  responseVariant[id][key] = value;
}
JsonArray SysModWeb::addResponseA(const char * id, const char * key) {
  JsonVariant responseVariant = getResponseDoc()->as<JsonVariant>();
  if (responseVariant[id].isNull()) responseVariant.createNestedObject(id);
  return responseVariant[id].createNestedArray(key);
}

void SysModWeb::clientsToJson(JsonArray array, bool nameOnly, const char * filter) {
  for (auto client:ws->getClients()) {
    if (nameOnly) {
      array.add((char *)client->remoteIP().toString().c_str()); //create a copy!
    } else {
      // print->print("Client %d %d %s\n", client->id(), client->queueIsFull(), client->remoteIP().toString().c_str());
      JsonArray row = array.createNestedArray();
      row.add(client->id());
      row.add((char *)client->remoteIP().toString().c_str()); //create a copy!
      row.add(client->queueIsFull());
      row.add(client->status());
    }
  }
}

JsonDocument * SysModWeb::getResponseDoc() {
  // print->print("response wsevent core %d %s\n", xPortGetCoreID(), pcTaskGetTaskName(NULL));

  return strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1;
}
