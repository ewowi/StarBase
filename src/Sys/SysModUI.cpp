/*
   @title     StarMod
   @file      SysModUI.cpp
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "SysModUI.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModModel.h"

#include "html_ui.h"

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<UCFun> SysModUI::ucFunctions;
std::vector<VarLoop> SysModUI::loopFunctions;
int SysModUI::varCounter = 1; //start with 1 so it can be negative, see var["o"]
bool SysModUI::varLoopsChanged = false;;

SysModUI::SysModUI() :SysModule("UI") {
  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  success &= web->addURL("/", "text/html", nullptr, PAGE_index, PAGE_index_L);
  // success &= web->addURL("/index.js", "application/javascript", nullptr, PAGE_indexJs, PAGE_indexJs_length);
  // success &= web->addURL("/app.js", "application/javascript", nullptr, PAGE_appJs, PAGE_appJs_length);
  // success &= web->addURL("/index.css", "text/css", "/index.css");

  success &= web->setupJsonHandlers("/json", processJson); //for websocket ("GET") and curl (POST)

  web->processURL("/json", web->serveJson);

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

//serve index.htm
void SysModUI::setup() {
  SysModule::setup();

  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = initModule(parentVar, name);

  JsonObject tableVar = initTable(parentVar, "vlTbl", nullptr, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Variable loops");
    web->addResponse(var["id"], "comment", "Loops initiated by a variable");
    JsonArray rows = web->addResponseA(var["id"], "table");

    for (auto varLoop = begin (loopFunctions); varLoop != end (loopFunctions); ++varLoop) {
      JsonArray row = rows.createNestedArray();
      row.add(varLoop->var["id"]);
      row.add(varLoop->counter);

      varLoopsChanged = varLoop->counter != varLoop->prevCounter;
      varLoop->prevCounter = varLoop->counter;
      varLoop->counter = 0;
    }
  });
  initText(tableVar, "vlVar", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
  });
  initNumber(tableVar, "vlLoopps", 0, 0, 999, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Loops p s");
  });

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModUI::loop() {
  // SysModule::loop();

  for (auto varLoop = begin (loopFunctions); varLoop != end (loopFunctions); ++varLoop) {
    if (millis() - varLoop->lastMillis >= varLoop->interval) {
      varLoop->lastMillis = millis();

      SysModWeb::ws->cleanupClients();
      if (SysModWeb::ws->count()) {
        SysModWeb::wsSendBytesCounter++;

        //send var to notify client data coming is for var (client then knows it is canvas and expects data for it)
        setChFunAndWs(varLoop->var, "new");

        //send leds info in binary data format
        //tbd: this can crash on 64*64 matrices...
        // USER_PRINTF(" %d\n", varLoop->bufSize);
        if (SysModWeb::ws->count() == 0) USER_PRINTF("%s ws count 0\n", __PRETTY_FUNCTION__);
        if (!SysModWeb::ws->enabled()) USER_PRINTF("%s ws not enabled\n", __PRETTY_FUNCTION__);
        AsyncWebSocketMessageBuffer * wsBuf = SysModWeb::ws->makeBuffer(varLoop->bufSize * 3 + 4);
        if (wsBuf) {//out of memory
          wsBuf->lock();
          uint8_t* buffer = wsBuf->get();

          //to loop over old size
          buffer[0] = varLoop->bufSize / 256;
          buffer[1] = varLoop->bufSize % 256;
          //buffer[2] can be removed
          // USER_PRINTF("interval1 %u %d %d %d %d %d %d\n", millis(), varLoop->interval, varLoop->bufSize, buffer[0], buffer[1]);

          varLoop->loopFun(varLoop->var, buffer); //call the function and fill the buffer

          varLoop->bufSize = buffer[0] * 256 + buffer[1];
          varLoop->interval = buffer[3]*10; //from cs to ms
          // USER_PRINTF("interval2 %u %d %d %d %d %d %d\n", millis(), varLoop->interval, varLoop->bufSize, buffer[0], buffer[1], buffer[2], buffer[3]);

          for (auto client:SysModWeb::ws->getClients()) {
            if (client->status() == WS_CONNECTED && !client->queueIsFull() && client->queueLength()<=1) //lossy
              client->binary(wsBuf);
            else {
              // web->clientsChanged = true; tbd: changed also if full status changes
              // print->printClient("loopFun skip frame", client);
            }
          }
          wsBuf->unlock();
          SysModWeb::ws->_cleanBuffers();
        }
        else {
          USER_PRINTF("loopFun WS buffer allocation failed\n");
          SysModWeb::ws->closeAll(1013); //code 1013 = temporary overload, try again later
          SysModWeb::ws->cleanupClients(0); //disconnect all clients to release memory
          SysModWeb::ws->_cleanBuffers();
        }
      }

      varLoop->counter++;
      // USER_PRINTF("%s %u %u %d %d\n", varLoop->var["id"].as<const char *>(), varLoop->lastMillis, millis(), varLoop->interval, varLoop->counter);
    }
  }

}

void SysModUI::loop1s() {
  //if something changed in vloops
  if (varLoopsChanged) {
    varLoopsChanged = false;

    processUiFun("vlTbl");
  }

}

JsonObject SysModUI::initVar(JsonObject parent, const char * id, const char * type, bool readOnly, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject var = mdl->findVar(id);

  //create new var
  if (var.isNull()) {
    USER_PRINTF("initVar create new %s: %s\n", type, id);
    if (parent.isNull()) {
      JsonArray vars = mdl->model->as<JsonArray>();
      var = vars.createNestedObject();
    } else {
      if (parent["n"].isNull()) parent.createNestedArray("n"); //if parent exist and no "n" array, create it
      var = parent["n"].createNestedObject();
      // serializeJson(model, Serial);Serial.println();
    }
    var["id"] = (char *)id; //create a copy!
  }
  else {
    USER_PRINT_NOT("Object %s already defined\n", id);
  }

  if (!var.isNull()) {
    if (var["type"] != type) 
      var["type"] = (char *)type; //create a copy!!
    if (var["ro"] != readOnly) 
      var["ro"] = readOnly;
    //readOnly's will be deleted, if not already so
    var["o"] = -varCounter++; //make order negative to check if not obsolete, see cleanUpModel

    //if uiFun, add it to the list
    if (uiFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      //lambda update: when replacing typedef void(*UCFun)(JsonObject); with typedef std::function<void(JsonObject)> UCFun; this gives error:
      //  mismatched types 'T*' and 'std::function<void(ArduinoJson::V6213PB2::JsonObject)>' { return *__it == _M_value; }
      //  it also looks like functions are not added more then once anyway
      // std::vector<UCFun>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), uiFun);
      // if (itr!=ucFunctions.end()) //found
      //   var["uiFun"] = distance(ucFunctions.begin(), itr); //assign found function
      // else { //not found
        ucFunctions.push_back(uiFun); //add new function
        var["uiFun"] = ucFunctions.size()-1;
      // }
    }

    //if chFun, add it to the list
    if (chFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      // std::vector<UCFun>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), chFun);
      // if (itr!=ucFunctions.end()) //found
      //   var["chFun"] = distance(ucFunctions.begin(), itr); //assign found function
      // else { //not found
        ucFunctions.push_back(chFun); //add new function
        var["chFun"] = ucFunctions.size()-1;
      // }
    }

    //if loopFun, add it to the list
    if (loopFun) {
      //no need to check if already in...
      VarLoop loop;
      loop.loopFun = loopFun;
      loop.var = var;

      loopFunctions.push_back(loop);
      var["loopFun"] = loopFunctions.size()-1;
      varLoopsChanged = true;
      // USER_PRINTF("iObject loopFun %s %u %u %d %d\n", var["id"].as<const char *>());
    }
  }
  else
    USER_PRINTF("initVar could not find or create var %s with %s\n", id, type);

  return var;
}

//tbd: use template T for value
//run the change function and send response to all? websocket clients
void SysModUI::setChFunAndWs(JsonObject var, const char * value) { //value: bypass var["value"]

  if (!var["chFun"].isNull()) {//isNull needed here!
    size_t funNr = var["chFun"];
    if (funNr < ucFunctions.size()) 
      ucFunctions[funNr](var);
    else    
      USER_PRINTF("setChFunAndWs function nr %s outside bounds %d >= %d\n", var["id"].as<const char *>(), funNr, ucFunctions.size());
  }

  JsonDocument *responseDoc = web->getResponseDoc();
  responseDoc->clear(); //needed for deserializeJson?
  JsonVariant responseVariant = responseDoc->as<JsonVariant>();

  if (value)
    web->addResponse(var["id"], "value", value);
  else {
    if (var["value"].is<int>())
      web->addResponseI(var["id"], "value", var["value"].as<int>());
    else if (var["value"].is<bool>())
      web->addResponseB(var["id"], "value", var["value"].as<bool>());
    else if (var["value"].is<const char *>())
      web->addResponse(var["id"], "value", var["value"].as<const char *>());
    else if (var["value"].is<JsonArray>())
      web->addResponseArray(var["id"], "value", var["value"].as<JsonArray>());
    else {
      USER_PRINTF("unknown type for %s\n", var["value"].as<String>().c_str());
      web->addResponse(var["id"], "value", var["value"]);
    }
    // if (var["id"] == "pview" || var["id"] == "fx") {
    //   print->printJson("setChFunAndWs response", responseDoc);
    // }
  }

  web->sendDataWs(responseVariant);
}

const char * SysModUI::processJson(JsonVariant &json) {
  if (json.is<JsonObject>()) //should be
  {
    for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
      const char * key = pair.key().c_str();
      JsonVariant value = pair.value();

      // commands
      if (pair.key() == "v") {
        // do nothing as it is no real var bu  the verbose command of WLED
        USER_PRINTF("processJson v type %s\n", pair.value().as<String>());
      }
      else if (pair.key() == "view") {
        // do nothing as it is no real var bu  the verbose command of WLED
        JsonObject var = mdl->findVar("System");
        USER_PRINTF("processJson view v:%s n: %d s:%s\n", pair.value().as<String>(), var.isNull(), var["id"].as<const char *>());
        var["view"] = (char *)value.as<const char *>(); //create a copy!
      }
      else if (pair.key() == "uiFun") { //JsonString can do ==
        //find the select var and collect it's options...
        if (value.is<JsonArray>()) { //should be
          for (JsonVariant value2: value.as<JsonArray>()) {
            JsonObject var = mdl->findVar(value2); //value is the id
            if (!var.isNull()) {
              //call ui function...
              if (!var["uiFun"].isNull()) {//isnull needed here!
                size_t funNr = var["uiFun"];
                if (funNr < ucFunctions.size())
                  ucFunctions[funNr](var);
                else    
                  USER_PRINTF("processJson function nr %s outside bounds %d >= %d\n", var["id"].as<const char *>(), funNr, ucFunctions.size());

                //if select var, send value back
                if (var["type"] == "select")
                  web->addResponseI(var["id"], "value", var["value"]); //temp assume int only

                // print->printJson("PJ Command", responseDoc);
              }
            }
            else
              USER_PRINTF("processJson Command %s var %s not found\n", key, value2.as<String>().c_str());
          }
        } else
          USER_PRINTF("processJson value not array? %s %s\n", key, value.as<String>().c_str());
      } 
      else { //normal change
        if (!value.is<JsonObject>()) { //no vars (inserted by uiFun responses)

          //check if we deal with multiple rows (from table type)
          char * rowNr = strtok((char *)key, "#");
          if (rowNr != NULL ) {
            key = rowNr;
            rowNr = strtok(NULL, " ");
          }

          JsonObject var = mdl->findVar(key);

          USER_PRINTF("processJson k:%s r:%s (%s == %s ? %d)\n", key, rowNr?rowNr:"na", var["value"].as<String>().c_str(), value.as<String>().c_str(), var["value"] == value);

          if (!var.isNull())
          {
            bool changed = false;
            //if we deal with multiple rows, value should be an array and check the corresponding array item
            //if value not array we change it anyway
            if (rowNr) {
              //var value should be array
              if (var["value"].is<JsonArray>())
                changed = var["value"][atoi(rowNr)] != value;
              else {
                print->printJson("we want an array for value but : ", var);
                changed = true; //we should change anyway
              }
            }
            else //normal situation
              changed = var["value"] != value;

            if (changed) {
              // USER_PRINTF("processJson %s %s->%s\n", key, var["value"].as<String>().c_str(), value.as<String>().c_str());

              //set new value
              if (value.is<const char *>())
                mdl->setValueC(key, value.as<const char *>());
              else if (value.is<bool>())
                mdl->setValueB(key, value.as<bool>(), rowNr?atoi(rowNr):-1);
              else if (value.is<int>())
                mdl->setValueI(key, value.as<int>());
              else {
                USER_PRINTF("processJson %s %s->%s not a supported type yet\n", key, var["value"].as<String>().c_str(), value.as<String>().c_str());
              }
            }
            else if (var["type"] == "button") //button always
              setChFunAndWs(var); //setValue without assignment
          }
          else
            USER_PRINTF("Object %s not found\n", key);
        }
      }
    } //for json pairs
  }
  else
    USER_PRINTF("Json not object???\n");
  return nullptr;
}

void SysModUI::processUiFun(const char * id) {
  JsonDocument *responseDoc = web->getResponseDoc();
  responseDoc->clear(); //needed for deserializeJson?
  JsonVariant responseVariant = responseDoc->as<JsonVariant>();

  JsonArray array = responseVariant.createNestedArray("uiFun");
  array.add(id);
  processJson(responseVariant); //this calls uiFun command, which might change varLoopsChanged
  //this also updates uiFun stuff - not needed!

  web->sendDataWs(*responseDoc);
}