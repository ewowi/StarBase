/*
   @title     StarMod
   @file      SysModWeb.cpp
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModWeb.h"
#include "SysModModel.h"
#include "SysModUI.h"
#include "SysModFiles.h"
#include "SysModules.h"

#include "User/UserModMDNS.h"

#include "html_ui.h"

#include "AsyncJson.h"

#include <ArduinoOTA.h>

//https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/
//https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/

SysModWeb::SysModWeb() :SysModule("Web") {
  //CORS compatiblity
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), "*");
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), "*");
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), "*");

  responseDocLoopTask = new JsonDocument; responseDocLoopTask->to<JsonObject>();
  responseDocAsyncTCP = new JsonDocument; responseDocAsyncTCP->to<JsonObject>();
};

void SysModWeb::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name, 3101);

  JsonObject tableVar = ui->initTable(parentVar, "clTbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Clients");
      return true;
    default: return false;
  }});

  ui->initNumber(tableVar, "clNr", UINT16_MAX, 0, 999, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun: {
      unsigned8 rowNr = 0; for (auto client:ws.getClients())
        mdl->setValue(var, client->id(), rowNr++);
      return true; }
    case f_UIFun:
      ui->setLabel(var, "Nr");
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "clIp", nullptr, 16, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun: {
      unsigned8 rowNr = 0; for (auto client:ws.getClients())
        mdl->setValue(var, JsonString(client->remoteIP().toString().c_str(), JsonString::Copied), rowNr++);
      return true; }
    case f_UIFun:
      ui->setLabel(var, "IP");
      return true;
    default: return false;
  }});

  ui->initCheckBox(tableVar, "clIsFull", UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun: {
      unsigned8 rowNr = 0; for (auto client:ws.getClients())
        mdl->setValue(var, client->queueIsFull(), rowNr++);
      return true; }
    case f_UIFun:
      ui->setLabel(var, "Is full");
      return true;
    default: return false;
  }});

  ui->initSelect(tableVar, "clStatus", UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun: {
      unsigned8 rowNr = 0; for (auto client:ws.getClients())
        mdl->setValue(var, client->status(), rowNr++);
      return true; }
    case f_UIFun:
    {
      ui->setLabel(var, "Status");
      //tbd: not working yet in ui
      JsonArray options = ui->setOptions(var);
      options.add("Disconnected"); //0
      options.add("Connected"); //1
      options.add("Disconnecting"); //2
      return true;
    }
    default: return false;
  }});

  ui->initNumber(tableVar, "clLength", UINT16_MAX, 0, WS_MAX_QUEUED_MESSAGES, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun: {
      unsigned8 rowNr = 0; for (auto client:ws.getClients())
        mdl->setValue(var, client->queueLength(), rowNr++);
      return true; }
    case f_UIFun:
      ui->setLabel(var, "Length");
      return true;
    default: return false;
  }});

  ui->initNumber(parentVar, "maxQueue", WS_MAX_QUEUED_MESSAGES, 0, WS_MAX_QUEUED_MESSAGES, true);

  ui->initText(parentVar, "wsSend", nullptr, 16, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "WS Send");
      // ui->setComment(var, "web socket calls");
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "wsRecv", nullptr, 16, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "WS Recv");
      // ui->setComment(var, "web socket calls");
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "udpSend", nullptr, 16, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "UDP Send");
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "udpRecv", nullptr, 16, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "UDP Recv");
      return true;
    default: return false;
  }});

}

void SysModWeb::loop() {
  // SysModule::loop();

  //currently not used as each variable is send individually
  if (this->modelUpdated) {
    sendDataWs(*mdl->model); //send new data, all clients, no def

    this->modelUpdated = false;
  }

  // if something changed in clTbl
  if (clientsChanged) {
    clientsChanged = false;

    USER_PRINTF("SysModWeb clientsChanged\n");
    for (JsonObject childVar: mdl->varChildren("clTbl"))
      ui->callVarFun(childVar, UINT8_MAX, f_ValueFun);
  }

}

void SysModWeb::loop1s() {
  for (JsonObject childVar: mdl->varChildren("clTbl"))
    ui->callVarFun(childVar, UINT8_MAX, f_ValueFun);

  mdl->setUIValueV("wsSend", "#: %d /s T: %d B/s B:%d B/s", sendWsCounter, sendWsTBytes, sendWsBBytes);
  sendWsCounter = 0;
  sendWsTBytes = 0;
  sendWsBBytes = 0;
  mdl->setUIValueV("wsRecv", "#: %d /s %d B/s", recvWsCounter, recvWsBytes);
  recvWsCounter = 0;
  recvWsBytes = 0;

  mdl->setUIValueV("udpSend", "#: %d /s %d B/s", sendUDPCounter, sendUDPBytes);
  sendUDPCounter = 0;
  sendUDPBytes = 0;
  mdl->setUIValueV("udpRecv", "#: %d /s %d B/s", recvUDPCounter, recvUDPBytes);
  recvUDPCounter = 0;
  recvUDPBytes = 0;

  sendResponseObject(); //this sends all the loopTask responses once per second !!!
}

void SysModWeb::reboot() {
  USER_PRINTF("SysModWeb reboot\n");
  ws.closeAll(1012);
}

void SysModWeb::connectedChanged() {
  if (mdls->isConnected) {
    #ifdef STARMOD_USE_Psychic

      server.listen(80);

      ws.onOpen(wsEventOpen);
      ws.onFrame(wsEventFrame);
      ws.onClose(wsEventClose);

      server.on("/ws")->setHandler(&ws);
    
    #else
      ws.onEvent( [this](WebSocket * server, WebClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {wsEvent(server, client, type, arg, data, len);});
      server.addHandler(&ws);
      server.begin();
    #endif

    server.on("/", HTTP_GET, [this](WebRequest *request) {serveIndex(request);});

    //serve json calls
    server.on("/json", HTTP_GET, [this](WebRequest *request) {serveJson(request);});

    server.addHandler(new AsyncCallbackJsonWebHandler("/json", [this](WebRequest *request, JsonVariant &json){jsonHandler(request, json);}));

    server.on("/update", HTTP_POST, [](WebRequest *) {}, [this](WebRequest *request, const String& filename, size_t index, byte *data, size_t len, bool final) {serveUpdate(request, filename, index, data, len, final);});
    server.on("/file", HTTP_GET, [this](WebRequest *request) {serveFiles(request);});
    server.on("/upload", HTTP_POST, [](WebRequest *) {}, [this](WebRequest *request, const String& filename, size_t index, byte *data, size_t len, bool final) {serveUpload(request, filename, index, data, len, final);});

    server.onNotFound([this](AsyncWebServerRequest *request) {
      USER_PRINTF("Not-Found HTTP call: URI: %s\n", request->url().c_str()); ///hotspot-detect.html
      if (this->captivePortal(request)) return;
    });


    // USER_PRINTF("%s server (re)started\n", name); //causes crash for some reason...
    USER_PRINTF("server (re)started\n");
  }
  //else remove handlers...
}

//WebSocket connection to 'ws://192.168.8.152/ws' failed: The operation couldn’t be completed. Protocol error
//WS error 192.168.8.126 9 (2)
//WS event data 0.0.0.0 (1) 0 0 0=0? 34 0
//WS pong 0.0.0.0 9 (1)
//wsEvent deserializeJson failed with code EmptyInput

// https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/examples/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser.ino
void SysModWeb::wsEvent(WebSocket * ws, WebClient * client, AwsEventType type, void * arg, byte *data, size_t len){
  // if (!ws.count()) {
  //   USER_PRINT_Async("wsEvent no clients\n");
  //   return;
  // }
  if (type == WS_EVT_CONNECT) {
    printClient("WS client connected", client);

    JsonArray model = mdl->model->as<JsonArray>();

    //inspired by https://github.com/bblanchon/ArduinoJson/issues/1280
    //store arrayindex and sort order in vector
    std::vector<ArrayIndexSortValue> aisvs;
    size_t index = 0;
    for (JsonObject moduleVar: model) {
      ArrayIndexSortValue aisv;
      aisv.index = index++;
      aisv.value = mdl->varOrder(moduleVar);
      aisvs.push_back(aisv);
    }
    //sort the vector by the order
    std::sort(aisvs.begin(), aisvs.end(), [](const ArrayIndexSortValue &a, const ArrayIndexSortValue &b) {return a.value < b.value;});

    //send model per module to stay under websocket size limit of 8192
    for (const ArrayIndexSortValue &aisv : aisvs) {
      sendDataWs(model[aisv.index], client); //send definition to client
    }

    clientsChanged = true;
  } else if (type == WS_EVT_DISCONNECT) {
    printClient("WS Client disconnected", client);
    clientsChanged = true;
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    // USER_PRINT_Async("  info %d %d %d=%d? %d %d\n", info->final, info->index, info->len, len, info->opcode, data[0]);
    if (info->final && info->index == 0 && info->len == len) { //not multipart
      recvWsCounter++;
      recvWsBytes+=len;
      printClient("WS event data", client);
      // the whole message is in a single frame and we got all of its data (max. 1450 bytes)
      if (info->opcode == WS_TEXT)
      {
        if (len > 0 && len < 10 && data[0] == 'p') {
          // application layer ping/pong heartbeat.
          // client-side socket layer ping packets are unresponded (investigate)
          // printClient("WS client pong", client); //crash?
          USER_PRINT_Async("pong\n");
          client->text("pong");
        } else {
          JsonDocument *responseDoc = web->getResponseDoc(); //we need the doc for deserializeJson
          JsonObject responseObject = web->getResponseObject();

          DeserializationError error = deserializeJson(*responseDoc, data, len); //data to responseDoc

          if (error || responseObject.isNull()) {
            USER_PRINT_Async("wsEvent deserializeJson failed with code %s\n", error.c_str());
            client->text("{\"success\":true}"); // we have to send something back otherwise WS connection closes
          } else {
            bool isUiFun = !responseObject["uiFun"].isNull();
            ui->processJson(responseObject); //adds to responseDoc / responseObject

            if (responseObject.size()) {
              web->sendResponseObject(isUiFun?client:nullptr); //uiFun only send to requesting client async response
            }
            else {
              USER_PRINT_Async("WS_EVT_DATA no responseDoc\n");
              client->text("{\"success\":true}"); // we have to send something back otherwise WS connection closes
            }
          }
        }
      }
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        // if(info->num == 0)
        //   USER_PRINT_Async("ws[%s][%u] %s-message start\n", ws.url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        // USER_PRINT_Async("ws[%s][%u] frame[%u] start[%llu]\n", ws.url(), client->id(), info->num, info->len);
        USER_PRINTF("💀");
      }

      // USER_PRINT_Async("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", ws.url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      USER_PRINTF("💀");

      if(info->opcode == WS_TEXT){
        for (size_t i=0; i < len; i++) {
          msg += (char) data[i];
        }
      }
      // else {
      //   char buff[3];
      //   for (size_t i=0; i < len; i++) {
      //     sprintf(buff, "%02x ", (unsigned8) data[i]);
      //     msg += buff ;
      //   }
      // }
      USER_PRINT_Async("%s\n",msg.c_str());

      if ((info->index + len) == info->len) {
        // USER_PRINT_Async("ws[%s][%u] frame[%u] end[%llu]\n", ws.url(), client->id(), info->num, info->len);
        USER_PRINTF("👻");
        if(info->final){
          // USER_PRINT_Async("ws[%s][%u] %s-message end\n", ws.url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          USER_PRINTF("☠️");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }

      //message is comprised of multiple frames or the frame is split into multiple packets
      //if(info->index == 0){
        //if (!wsFrameBuffer && len < 4096) wsFrameBuffer = new unsigned8[4096];
      //}

      //if (wsFrameBuffer && len < 4096 && info->index + info->)
      //{

      //}

      // if((info->index + len) == info->len){
      //   if(info->final){
      //     if(info->message_opcode == WS_TEXT) {
      //       client->text("{\"error\":9}"); //we do not handle split packets right now
      //       USER_PRINT_Async("WS multipart message: we do not handle split packets right now\n");
      //     }
      //   }
      // }
      // USER_PRINT_Async("WS multipart message f:%d i:%d len:%d == %d\n", info->final, info->index, info->len, len);
    }
  } else if (type == WS_EVT_ERROR){
    //error was received from the other end
    // printClient("WS error", client); //crashes
    // USER_PRINT_Async("WS error\n");
    USER_PRINT_Async("ws[%s][%u] error(): \n", ws->url(), client->id());//, *((unsigned16*)arg));//, (char*)data);
  } else if (type == WS_EVT_PONG){
    //pong message was received (in response to a ping request maybe)
    // printClient("WS pong", client); //crashes!
    // USER_PRINT_Async("WS pong\n");
    // USER_PRINT_Async("ws[%s][%u] pong[%u]: %s\n", ws.url(), client->id(), len, (len)?(char*)data:"");
    USER_PRINT_Async("ws[%s][%u] pong[%u]: \n", ws->url(), client->id(), len);//, (len)?(char*)data:"");
  }
}

void SysModWeb::sendDataWs(JsonVariant json, WebClient * client) {

  size_t len = measureJson(json);
  sendDataWs([json, len](AsyncWebSocketMessageBuffer * wsBuf) {
    serializeJson(json, wsBuf->get(), len);
  }, len, false, client); //false -> text
}

//https://kcwong-joe.medium.com/passing-a-function-as-a-parameter-in-c-a132e69669f6
void SysModWeb::sendDataWs(std::function<void(AsyncWebSocketMessageBuffer *)> fill, size_t len, bool isBinary, WebClient * client) {

  xSemaphoreTake(wsMutex, portMAX_DELAY);

  ws.cleanupClients(); //only if above threshold

  if (ws.count()) {
    if (len > 8192)
      USER_PRINTF("program error: sendDataWs BufferLen too high !!!%d\n", len);
    AsyncWebSocketMessageBuffer * wsBuf = ws.makeBuffer(len); //assert failed: block_trim_free heap_tlsf.c:371 (block_is_free(block) && "block must be free"), AsyncWebSocket::makeBuffer(unsigned int)

    if (wsBuf) {
      wsBuf->lock();

      fill(wsBuf); //function parameter

      for (auto loopClient:ws.getClients()) {
        if (!client || client == loopClient) {
          if (loopClient->status() == WS_CONNECTED && !loopClient->queueIsFull()) { //WS_MAX_QUEUED_MESSAGES / ws.count() / 2)) { //binary is lossy
            if ((!isBinary || loopClient->queueLength() <= 3)) {
              isBinary?loopClient->binary(wsBuf): loopClient->text(wsBuf);
              sendWsCounter++;
              if (isBinary)
                sendWsBBytes+=len;
              else 
                sendWsTBytes+=len;
            }
          }
          else {
            printClient("sendDataWs client full or not connected", loopClient);
            // USER_PRINTF("sendDataWs client full or not connected\n");
            ws.cleanupClients(); //only if above threshold
            ws._cleanBuffers();
          }
        }
      }
      wsBuf->unlock();
      ws._cleanBuffers();
    }
    else {
      USER_PRINT_Async("sendDataWs WS buffer allocation failed\n");
      ws.closeAll(1013); //code 1013 = temporary overload, try again later
      ws.cleanupClients(0); //disconnect ALL clients to release memory
      ws._cleanBuffers();
    }
  }

  xSemaphoreGive(wsMutex);
}

//add an url to the webserver to listen to
void SysModWeb::serveIndex(WebRequest *request) {

  USER_PRINT_Async("Webserver: server.on serveIndex csdata %d-%d (%s)", PAGE_index, PAGE_index_L, request->url().c_str());

  if (captivePortal(request)) return;

  // if (handleIfNoneMatchCacheHeader(request)) return;

  WebResponse *response;
  response = request->beginResponse_P(200, "text/html", PAGE_index, PAGE_index_L);
  response->addHeader(FPSTR("Content-Encoding"),"gzip");
  // setStaticContentCacheHeaders(response);
  request->send(response);

  USER_PRINT_Async("!\n");
}

void SysModWeb::serveUpload(WebRequest *request, const String& filename, size_t index, byte *data, size_t len, bool final) {

  // curl -F 'data=@fixture1.json' 192.168.8.213/upload
  USER_PRINT_Async("serveUpload r:%s f:%s i:%d l:%d f:%d\n", request->url().c_str(), filename.c_str(), index, len, final);

  mdl->setValue("upload", index/10000);
  web->sendResponseObject(); //otherwise not send in asyn_tcp thread

  if (!index) {
    String finalname = filename;
    if (finalname.charAt(0) != '/') {
      finalname = '/' + finalname; // prepend slash if missing
    }

    request->_tempFile = files->open(finalname.c_str(), "w");
    // DEBUG_PRINT("Uploading ");
    // DEBUG_PRINTLN(finalname);
    // if (finalname.equals("/presets.json")) presetsModifiedTime = toki.second();
  }
  if (len) {
    request->_tempFile.write(data,len);
  }
  if (final) {
    request->_tempFile.close();

    mdl->setValue("upload", UINT16_MAX - 10); //success
    web->sendResponseObject(); //otherwise not send in asyn_tcp thread

    request->send(200, "text/plain", F("File Uploaded!"));

    files->filesChanged = true;
  }
}

void SysModWeb::serveUpdate(WebRequest *request, const String& filename, size_t index, byte *data, size_t len, bool final) {

  // curl -F 'data=@fixture1.json' 192.168.8.213/upload
  // USER_PRINT_Async("serveUpdate r:%s f:%s i:%d l:%d f:%d\n", request->url().c_str(), filename.c_str(), index, len, final);
  
  mdl->setValue("update", index/10000);
  web->sendResponseObject(); //otherwise not send in asyn_tcp thread

  if (!index) {
    USER_PRINTF("OTA Update Start\n");
    // WLED::instance().disableWatchdog();
    // usermods.onUpdateBegin(true); // notify usermods that update is about to begin (some may require task de-init)
    // lastEditTime = millis(); // make sure PIN does not lock during update
    Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
  }

  if (!Update.hasError()) 
    Update.write(data, len);
  else {
    mdl->setValue("update", UINT16_MAX - 20); //fail
    web->sendResponseObject(); //otherwise not send in asyn_tcp thread
  }

  if (final) {
    bool success = Update.end(true);
    mdl->setValue("update", success?UINT16_MAX - 10:UINT16_MAX - 20);
    web->sendResponseObject(); //otherwise not send in asyn_tcp thread

    char message[64];
    const char * instanceName = mdl->getValue("instanceName");

    print->fFormat(message, sizeof(message)-1, "Update of %s (...%d) %s", instanceName, WiFi.localIP()[3], success?"Successful":"Failed");

    USER_PRINTF("%s\n", message);
    request->send(200, "text/plain", message);

    // usermods.onUpdateBegin(false); // notify usermods that update has failed (some may require task init)
    // WLED::instance().enableWatchdog();
  }
}

void SysModWeb::serveFiles(WebRequest *request) {

  const char * urlString = request->url().c_str();
  const char * path = urlString + strlen("/file"); //remove the uri from the path (skip their positions)
  USER_PRINT_Async("fileServer request %s\n", path);
  if(LittleFS.exists(path)) {
    request->send(LittleFS, path, "text/plain");//"application/json");
  }
}

void SysModWeb::jsonHandler(WebRequest *request, JsonVariant json) {

  print->printJson("jsonHandler", json);

  JsonObject responseObject = web->getResponseObject();

  ui->processJson(json);

  //WLED compatibility
  if (json["v"]) { //WLED compatibility: verbose response
    serveJson (request); //includes values just updated by processJson e.g. Bri
  }
  else {
  
    if (responseObject.size()) { //responseObject set by processJson e.g. uiFun

      char resStr[200];
      serializeJson(responseObject, resStr, 200);
      USER_PRINT_Async("processJsonUrl response %s\n", resStr);
      request->send(200, "application/json", resStr);

    }
    else
      // request->send(200, "text/plain", "OK");
      request->send(200, "application/json", F("{\"success\":true}"));
  }

  web->sendResponseObject();
}

void SysModWeb::clientsToJson(JsonArray array, bool nameOnly, const char * filter) {
  for (auto client:ws.getClients()) {
    if (nameOnly) {
      array.add(JsonString(client->remoteIP().toString().c_str(), JsonString::Copied));
    } else {
      // USER_PRINTF("Client %d %d ...%d\n", client->id(), client->queueIsFull(), client->remoteIP()[3]);
      JsonArray row = array.add<JsonArray>();
      row.add(client->id());
      array.add(JsonString(client->remoteIP().toString().c_str(), JsonString::Copied));
      row.add(client->queueIsFull());
      row.add(client->status());
      row.add(client->queueLength());
    }
  }
}

bool SysModWeb::captivePortal(WebRequest *request)
{
  USER_PRINTF("captivePortal %d %d\n", WiFi.localIP()[3], request->client()->localIP()[3]);

  if (ON_STA_FILTER(request)) return false; //only serve captive in AP mode
  String hostH;
  if (!request->hasHeader("Host")) return false;
  hostH = request->getHeader("Host")->value();

  if (!isIp(hostH) && hostH.indexOf(mdns->cmDNS) < 0) { //&& hostH.indexOf("wled.me") < 0
    USER_PRINTF("Captive portal\n");
    WebResponse *response = request->beginResponse(302);
    response->addHeader(F("Location"), F("http://4.3.2.1"));
    request->send(response);
    return true;
  }
  return false;
}

JsonDocument * SysModWeb::getResponseDoc() {
  // USER_PRINTF("response wsevent core %d %s\n", xPortGetCoreID(), pcTaskGetTaskName(NULL));

  return strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) == 0?web->responseDocLoopTask:web->responseDocAsyncTCP;
}

JsonObject SysModWeb::getResponseObject() {
  return getResponseDoc()->as<JsonObject>();
}

void SysModWeb::sendResponseObject(WebClient * client) {
  JsonObject responseObject = getResponseObject();
  if (responseObject.size()) {
    if (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0) {
      print->printJson("sendResponseObject", responseObject);
      print->printJDocInfo("  info", responseObject);
    }
    web->sendDataWs(responseObject, client);
    getResponseDoc()->to<JsonObject>(); //recreate!
  }
}

void SysModWeb::serveJson(WebRequest *request) {

  AsyncJsonResponse * response;

  // return model.json
  if (request->url().indexOf("mdl") > 0) {
    JsonArray model = mdl->model->as<JsonArray>();
    USER_PRINTF("serveJson model ...%d, %s %d %d\n", request->client()->remoteIP()[3], request->url().c_str(), model.size(),  measureJson(model));

    response = new AsyncJsonResponse(true); //array, removed size as ArduinoJson v7 doesnt care (tbd: here copy is mode, see WLED for using reference)
    JsonArray root = response->getRoot();

    root.set(model);
  } else { //WLED compatible
    USER_PRINTF("serveJson ...%d, %s\n", request->client()->remoteIP()[3], request->url().c_str());
    response = new AsyncJsonResponse(false); //object. removed size as ArduinoJson v7 doesnt care
    JsonObject root = response->getRoot();

    //temporary set all WLED variables (as otherwise WLED-native does not show the instance): tbd: clean up (state still needed, info not)
    const char* jsonState = "{\"transition\":7,\"ps\":9,\"pl\":-1,\"nl\":{\"on\":false,\"dur\":60,\"mode\":1,\"tbri\":0,\"rem\":-1},\"udpn\":{\"send\":false,\"recv\":true},\"lor\":0,\"mainseg\":0,\"seg\":[{\"id\":0,\"start\":0,\"stop\":144,\"len\":144,\"grp\":1,\"spc\":0,\"of\":0,\"on\":true,\"frz\":false,\"bri\":255,\"cct\":127,\"col\":[[182,15,98,0],[0,0,0,0],[255,224,160,0]],\"fx\":0,\"sx\":128,\"ix\":128,\"pal\":11,\"c1\":8,\"c2\":20,\"c3\":31,\"sel\":true,\"rev\":false,\"mi\":false,\"o1\":false,\"o2\":false,\"o3\":false,\"ssim\":0,\"mp12\":1}]}";
    JsonDocument docState;
    deserializeJson(docState, jsonState);
    root["state"] = docState;

    root["state"]["bri"] = mdl->getValue("bri");
    root["state"]["on"] = mdl->getValue("on").as<bool>();
    root["info"]["name"] = mdl->getValue("instanceName");
    root["info"]["arch"] = "esp32"; //platformName

    root["info"]["rel"] = "StarMod";
    root["info"]["ver"] = "0.0.1";
    root["info"]["vid"] = mdl->getValue("version").as<unsigned32>(); //WLED-native needs int otherwise status offline!!!
    root["info"]["leds"]["count"] = 999;
    root["info"]["leds"]["countP"] = 998;
    root["info"]["leds"]["fps"] = mdl->getValue("fps"); //tbd: should be realFps but is ro var
    root["info"]["wifi"]["rssi"] = WiFi.RSSI();// mdl->getValue("rssi"); (ro)

    root["info"]["mac"] = JsonString(mdns->escapedMac.c_str(), JsonString::Copied);
    root["info"]["ip"] = JsonString(WiFi.localIP().toString().c_str(), JsonString::Copied);
    // print->printJson("serveJson", root);
  }

  response->setLength();
  request->send(response);
} //serveJson