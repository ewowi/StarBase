/*
   @title     StarMod
   @file      SysModUI.cpp
   @date      20230807
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "SysModUI.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModModel.h"

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(JsonObject var)> SysModUI::ucFunctions;
std::vector<VarLoop> SysModUI::loopFunctions;
int SysModUI::varCounter = 1; //start with 1 so it can be negative, see var["o"]

bool SysModUI::varLoopsChanged = false;;

SysModUI::SysModUI() :Module("UI") {
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  success &= web->addURL("/", "/index.htm", "text/html");
  // success &= web->addURL("/index.js", "/index.js", "text/javascript");
  // success &= web->addURL("/index.css", "/index.css", "text/css");

  success &= web->setupJsonHandlers("/json", processJson);

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
};

//serve index.htm
void SysModUI::setup() {
  Module::setup();

  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = initModule(parentVar, name);

  JsonObject tableVar = initTable(parentVar, "vloops", nullptr, false, [](JsonObject var) { //uiFun
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
  initText(tableVar, "ulVar", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
  });
  initNumber(tableVar, "ulLoopps", -1, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Loops//s");
  });

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModUI::loop() {
  // Module::loop();

  for (auto varLoop = begin (loopFunctions); varLoop != end (loopFunctions); ++varLoop) {
    if (millis() - varLoop->lastMillis >= varLoop->interval) {
      varLoop->lastMillis = millis();

      SysModWeb::ws->cleanupClients();
      if (SysModWeb::ws->count()) {
        //check if there are valid clients before!!!
        bool okay = false;
        for (auto client:SysModWeb::ws->getClients()) {
          if (client->status() == WS_CONNECTED && !client->queueIsFull()) 
            okay = true;
        }

        if (okay) {

        SysModWeb::wsSendBytesCounter++;

        //send var to notify client data coming is for var (client then knows it is canvas and expects data for it)
        setChFunAndWs(varLoop->var, "new");

        //send leds info in binary data format
        //tbd: this can crash on 64*64 matrices...
        AsyncWebSocketMessageBuffer * wsBuf = SysModWeb::ws->makeBuffer(varLoop->bufSize * 3 + 4);
        if (wsBuf) {//out of memory
          wsBuf->lock();
          uint8_t* buffer = wsBuf->get();

          //to loop over old size
          buffer[0] = varLoop->bufSize / 256;
          buffer[1] = varLoop->bufSize % 256;
          //buffer[2] can be removed
          // print->print("interval1 %u %d %d %d %d %d %d\n", millis(), varLoop->interval, varLoop->bufSize, buffer[0], buffer[1]);

          varLoop->loopFun(varLoop->var, buffer); //call the function and fill the buffer

          varLoop->bufSize = buffer[0] * 256 + buffer[1];
          varLoop->interval = buffer[3]*10; //from cs to ms
          // print->print("interval2 %u %d %d %d %d %d %d\n", millis(), varLoop->interval, varLoop->bufSize, buffer[0], buffer[1], buffer[2], buffer[3]);

          for (auto client:SysModWeb::ws->getClients()) {
            if (client->status() == WS_CONNECTED && !client->queueIsFull()) 
              client->binary(wsBuf);
            else {
              // web->clientsChanged = true; tbd: changed also if full status changes
              web->printClient("loopFun client full or not connected", client);
            }
          }
          wsBuf->unlock();
          SysModWeb::ws->_cleanBuffers();
        }
        else {
          print->print("loopFun WS buffer allocation failed\n");
          SysModWeb::ws->closeAll(1013); //code 1013 = temporary overload, try again later
          SysModWeb::ws->cleanupClients(0); //disconnect all clients to release memory
          SysModWeb::ws->_cleanBuffers();
        }
        } // if okay
      }

      varLoop->counter++;
      // print->print("%s %u %u %d %d\n", varLoop->var["id"].as<const char *>(), varLoop->lastMillis, millis(), varLoop->interval, varLoop->counter);
    }
  }

  if (millis() - secondMillis >= 1000 || !secondMillis) {
    secondMillis = millis();

    //if something changed in vloops
    if (varLoopsChanged) {
      varLoopsChanged = false;

      processUiFun("vloops");
    }
  }
}

JsonObject SysModUI::initVar(JsonObject parent, const char * id, const char * type, bool readOnly, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject var = mdl->findVar(id);

  //create new var
  if (var.isNull()) {
    print->print("initVar create new %s: %s\n", type, id);
    if (parent.isNull()) {
      JsonArray vars = SysModModel::model->as<JsonArray>();
      var = vars.createNestedObject();
    } else {
      if (parent["n"].isNull()) parent.createNestedArray("n"); //if parent exist and no "n" array, create it
      var = parent["n"].createNestedObject();
      // serializeJson(model, Serial);Serial.println();
    }
    var["id"] = (char *)id; //copy!!
  }
  else
    print->print("Object %s already defined\n", id);

  if (!var.isNull()) {
    if (var["type"] != type) 
      var["type"] = (char *)type; //copy!!
    if (var["ro"] != readOnly) 
      var["ro"] = readOnly;
    //readOnly's will be deleted, if not already so
    var["o"] = -varCounter++; //make order negative to check if not obsolete, see cleanUpModel

    //if uiFun, add it to the list
    if (uiFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      std::vector<void(*)(JsonObject var)>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), uiFun);
      if (itr!=ucFunctions.end()) //found
        var["uiFun"] = distance(ucFunctions.begin(), itr); //assign found function
      else { //not found
        ucFunctions.push_back(uiFun); //add new function
        var["uiFun"] = ucFunctions.size()-1;
      }
    }

    //if chFun, add it to the list
    if (chFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      std::vector<void(*)(JsonObject var)>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), chFun);
      if (itr!=ucFunctions.end()) //found
        var["chFun"] = distance(ucFunctions.begin(), itr); //assign found function
      else { //not found
        ucFunctions.push_back(chFun); //add new function
        var["chFun"] = ucFunctions.size()-1;
      }
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
      // print->print("iObject loopFun %s %u %u %d %d\n", var["id"].as<const char *>());
    }
  }
  else
    print->print("initVar could not find or create var %s with %s\n", id, type);

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
      print->print("setChFunAndWs function nr %s outside bounds %d >= %d\n", var["id"].as<const char *>(), funNr, ucFunctions.size());
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
    else {
      print->print("unknown type for %s\n", var["value"].as<String>().c_str());
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
      if (pair.key() == "uiFun") { //JsonString can do ==
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
                  print->print("processJson function nr %s outside bounds %d >= %d\n", var["id"].as<const char *>(), funNr, ucFunctions.size());
                if (var["type"] == "select")
                  web->addResponseI(var["id"], "value", var["value"]); //temp assume int only

                // print->printJson("PJ Command", responseDoc);
              }
            }
            else
              print->print("processJson Command %s var %s not found\n", key, value2.as<String>().c_str());
          }
        } else
          print->print("processJson value not array?\n", key, value.as<String>().c_str());
      } 
      else { //normal change
        if (!value.is<JsonObject>()) { //no vars (inserted by uiFun responses)
          JsonObject var = mdl->findVar(key);
          if (!var.isNull())
          {
            if (var["value"] != value) { // if changed
              // print->print("processJson %s %s->%s\n", key, var["value"].as<String>().c_str(), value.as<String>().c_str());

              //set new value
              if (value.is<const char *>())
                mdl->setValueC(key, value.as<const char *>());
              else if (value.is<bool>())
                mdl->setValueB(key, value.as<bool>());
              else if (value.is<int>())
                mdl->setValueI(key, value.as<int>());
              else {
                print->print("processJson %s %s->%s not a supported type yet\n", key, var["value"].as<String>().c_str(), value.as<String>().c_str());
              }
            }
            else if (var["type"] == "button") //button always
              setChFunAndWs(var); //setValue without assignment
          }
          else
            print->print("Object %s not found\n", key);
        }
      }
    } //for json pairs
  }
  else
    print->print("Json not object???\n");
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