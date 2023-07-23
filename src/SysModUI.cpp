#include "SysModUI.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModModel.h"

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(JsonObject object)> SysModUI::ucFunctions;
std::vector<ObjectLoop> SysModUI::loopFunctions;
int SysModUI::objectCounter = 1; //start with 1 so it can be negative, see object["o"]

bool SysModUI::objectLoopsChanged = false;;

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

  parentObject = initGroup(parentObject, name);

  JsonObject objectLoopsObject = initMany(parentObject, "oloops", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "Object loops");
    web->addResponse(object["id"], "comment", "Loops initiated by an object");
    JsonArray rows = web->addResponseA(object["id"], "many");

    for (auto objectLoop = begin (loopFunctions); objectLoop != end (loopFunctions); ++objectLoop) {
      JsonArray row = rows.createNestedArray();
      row.add(objectLoop->object["id"]);
      row.add(objectLoop->counter);
      objectLoopsChanged = objectLoop->counter != objectLoop->prevCounter;
      objectLoop->prevCounter = objectLoop->counter;
      objectLoop->counter = 0;
    }
  });
  initDisplay(objectLoopsObject, "ulObject", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "Name");
  });
  initDisplay(objectLoopsObject, "ulLoopps", nullptr, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "Loops/s");
  });

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModUI::loop() {
  // Module::loop();

  for (auto objectLoop = begin (loopFunctions); objectLoop != end (loopFunctions); ++objectLoop) {
    if (millis() - objectLoop->lastMillis >= objectLoop->interval) {
      objectLoop->lastMillis = millis();

      web->ws->cleanupClients();
      if (web->ws->count()) {

        //send object to notify client data coming is for object (client then knows it is canvas and expects data for it)
        setChFunAndWs(objectLoop->object, "new");

        //send leds info in binary data format
        AsyncWebSocketMessageBuffer * wsBuf = web->ws->makeBuffer(objectLoop->bufSize * 3 + 4);
        if (wsBuf) {//out of memory
          wsBuf->lock();
          uint8_t* buffer = wsBuf->get();

          //to loop over old size
          buffer[0] = objectLoop->bufSize / 256;
          buffer[1] = objectLoop->bufSize % 256;
          //buffer[2] can be removed
          // print->print("interval1 %u %d %d %d %d %d %d\n", millis(), objectLoop->interval, objectLoop->bufSize, buffer[0], buffer[1]);

          objectLoop->loopFun(objectLoop->object, buffer); //call the function and fill the buffer

          objectLoop->bufSize = buffer[0] * 256 + buffer[1];
          objectLoop->interval = buffer[3]*10; //from cs to ms
          // print->print("interval2 %u %d %d %d %d %d %d\n", millis(), objectLoop->interval, objectLoop->bufSize, buffer[0], buffer[1], buffer[2], buffer[3]);

          for (auto client:web->ws->getClients()) {
            if (!client->queueIsFull() && client->status() == WS_CONNECTED) 
              client->binary(wsBuf);
            else {
              // web->clientsChanged = true; tbd: changed also if full status changes
              web->printClient("loopFun client full or not connected", client);
            }
          }
          wsBuf->unlock();
          web->ws->_cleanBuffers();
        }
        else {
          print->print("loopFun WS buffer allocation failed\n");
          web->ws->closeAll(1013); //code 1013 = temporary overload, try again later
          web->ws->cleanupClients(0); //disconnect all clients to release memory
          web->ws->_cleanBuffers();
        }
      }

      objectLoop->counter++;
      // print->print("%s %u %u %d %d\n", objectLoop->object["id"].as<const char *>(), objectLoop->lastMillis, millis(), objectLoop->interval, objectLoop->counter);
    }
  }

  if (millis() - secondMillis >= 1000 || !secondMillis) {
    secondMillis = millis();

    //if something changed in oloops
    if (objectLoopsChanged) {
      objectLoopsChanged = false;

      processUiFun("oloops");
    }
  }
}

JsonObject SysModUI::initGroup(JsonObject parent, const char * id, const char * value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "group", uiFun, chFun, loopFun);
  if (object["value"].isNull() && value) object["value"] = value;
  if (chFun && value) chFun(object);
  return object;
}

JsonObject SysModUI::initMany(JsonObject parent, const char * id, const char * value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "many", uiFun, chFun, loopFun);
  if (object["value"].isNull() && value) object["value"] = value;
  if (chFun && value) chFun(object);
  return object;
}

JsonObject SysModUI::initInput(JsonObject parent, const char * id, const char * value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "input", uiFun, chFun, loopFun);
  if (object["value"].isNull() && value) object["value"] = value;
  if (chFun && value) chFun(object);
  return object;
}

JsonObject SysModUI::initPassword(JsonObject parent, const char * id, const char * value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "password", uiFun, chFun, loopFun);
  if (object["value"].isNull() && value) object["value"] = value;
  if (chFun && value) chFun(object);
  return object;
}

JsonObject SysModUI::initNumber(JsonObject parent, const char * id, int value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "number", uiFun, chFun, loopFun);
  if (object["value"].isNull()) object["value"] = value;
  if (chFun) chFun(object);
  return object;
}

JsonObject SysModUI::initSlider(JsonObject parent, const char * id, int value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "range", uiFun, chFun, loopFun);
  if (object["value"].isNull()) object["value"] = value;
  if (chFun) chFun(object);
  return object;
}

JsonObject SysModUI::initCanvas(JsonObject parent, const char * id, int value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "canvas", uiFun, chFun, loopFun);
  if (object["value"].isNull()) object["value"] = value;
  if (chFun) chFun(object);
  return object;
}

JsonObject SysModUI::initDisplay(JsonObject parent, const char * id, const char * value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "display", uiFun, chFun, loopFun);
  if (object["value"].isNull() && value) object["value"] = value;
  if (chFun && value) chFun(object);
  return object;
}

JsonObject SysModUI::initCheckBox(JsonObject parent, const char * id, bool value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "checkbox", uiFun, chFun, loopFun);
  if (object["value"].isNull()) object["value"] = value;
  if (chFun) chFun(object);
  return object;
}

JsonObject SysModUI::initButton(JsonObject parent, const char * id, const char * value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "button", uiFun, chFun, loopFun);
  if (object["value"].isNull()) object["value"] = value;
  //no call of fun for buttons!!! 
  // if (chFun) chFun(object);
  return object;
}

JsonObject SysModUI::initDropdown(JsonObject parent, const char * id, uint8_t value, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = initObject(parent, id, "dropdown", uiFun, chFun, loopFun);
  if (object["value"].isNull()) object["value"] = value;
  if (chFun) chFun(object);
  return object;
}

JsonObject SysModUI::initObject(JsonObject parent, const char * id, const char * type, UCFun uiFun, UCFun chFun, LoopFun loopFun) {
  JsonObject object = mdl->findObject(id);

  //create new object
  if (object.isNull()) {
    print->print("initObject create new %s: %s\n", type, id);
    if (parent.isNull()) {
      JsonArray objects = mdl->model->as<JsonArray>();
      object = objects.createNestedObject();
    } else {
      if (parent["n"].isNull()) parent.createNestedArray("n"); //if parent exist and no "n" array, create it
      object = parent["n"].createNestedObject();
      // serializeJson(model, Serial);Serial.println();
    }
    object["id"] = id;
  }
  else
    print->print("Object %s already defined\n", id);

  if (!object.isNull()) {
    object["type"] = type;
    object["o"] = -objectCounter++; //make order negative to check if not obsolete, see cleanUpModel
    if (uiFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      std::vector<void(*)(JsonObject object)>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), uiFun);
      if (itr!=ucFunctions.end()) //found
        object["uiFun"] = distance(ucFunctions.begin(), itr); //assign found function
      else { //not found
        ucFunctions.push_back(uiFun); //add new function
        object["uiFun"] = ucFunctions.size()-1;
      }
    }
    if (chFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      std::vector<void(*)(JsonObject object)>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), chFun);
      if (itr!=ucFunctions.end()) //found
        object["chFun"] = distance(ucFunctions.begin(), itr); //assign found function
      else { //not found
        ucFunctions.push_back(chFun); //add new function
        object["chFun"] = ucFunctions.size()-1;
      }
    }
    if (loopFun) {
      ObjectLoop loop;
      loop.loopFun = loopFun;
      loop.object = object;

      loopFunctions.push_back(loop);
      object["loopFun"] = loopFunctions.size()-1;
      objectLoopsChanged = true;
      // print->print("iObject loopFun %s %u %u %d %d\n", object["id"].as<const char *>());
    }
  }
  else
    print->print("initObject could not find or create object %s with %s\n", id, type);

  return object;
}

//run the change function and send response to all? websocket clients
void SysModUI::setChFunAndWs(JsonObject object, const char * value) { //value: bypass object["value"]

  if (!object["chFun"].isNull()) {//isNull needed here!
    size_t funNr = object["chFun"];
    if (funNr < ucFunctions.size()) 
      ucFunctions[funNr](object);
    else    
      print->print("setChFunAndWs function nr %s outside bounds %d >= %d\n", object["id"].as<const char *>(), funNr, ucFunctions.size());
  }

  JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->as<JsonVariant>();
  (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->clear();

  if (value)
    web->addResponse(object["id"], "value", value);
  else {
    if (object["value"].is<int>())
      web->addResponseI(object["id"], "value", object["value"].as<int>());
    else if (object["value"].is<bool>())
      web->addResponseB(object["id"], "value", object["value"].as<bool>());
    else if (object["value"].is<const char *>())
      web->addResponse(object["id"], "value", object["value"].as<const char *>());
    else {
      print->print("unknown type for %s\n", object["value"].as<String>().c_str());
      web->addResponse(object["id"], "value", object["value"]);
    }
    // if (object["id"] == "pview" || object["id"] == "fx") {
    //   print->printJson("setChFunAndWs response", responseDoc);
    // }
  }

  web->sendDataWs(responseVariant);
}

const char * SysModUI::processJson(JsonVariant &json) { //static for setupJsonHandlers
  if (json.is<JsonObject>()) //should be
  {
    for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
      const char * key = pair.key().c_str();
      JsonVariant value = pair.value();

      // commands
      if (pair.key() == "uiFun") { //JsonString can do ==
        //find the dropdown object and collect it's options...
        JsonObject object = mdl->findObject(value); //value is the id
        if (!object.isNull()) {
          //call ui function...
          if (!object["uiFun"].isNull()) {//isnull needed here!
            size_t funNr = object["uiFun"];
            if (funNr < ucFunctions.size()) 
              ucFunctions[funNr](object);
            else    
              print->print("processJson function nr %s outside bounds %d >= %d\n", object["id"].as<const char *>(), funNr, ucFunctions.size());
            if (object["type"] == "dropdown")
              web->addResponseI(object["id"], "value", object["value"]); //temp assume int only

            // print->printJson("PJ Command", responseDoc);
          }
        }
        else
          print->print("PJ Command %s object %s not found\n", key, value.as<String>().c_str());
      } 
      else { //normal change
        if (!value.is<JsonObject>()) { //no objects (inserted by uiFun responses)
          JsonObject object = mdl->findObject(key);
          if (!object.isNull())
          {
            if (object["value"] != value) { // if changed
              // print->print("processJson %s %s->%s\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());

              //set new value
              if (value.is<const char *>())
                mdl->setValueC(key, value.as<const char *>());
              else if (value.is<bool>())
                mdl->setValueB(key, value.as<bool>());
              else if (value.is<int>())
                mdl->setValueI(key, value.as<int>());
              else {
                print->print("processJson %s %s->%s not a supported type yet\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());
              }
            }
            else if (object["type"] == "button") //button always
              setChFunAndWs(object); //setValue without assignment
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
  JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->as<JsonVariant>();
  (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->clear(); //needed for deserializeJson?

  responseVariant["uiFun"] = id;
  processJson(responseVariant); //this calls uiFun command, which might change objectLoopsChanged
  //this also updates uiFun stuff - not needed!

  // print->printJson("oloops change response", responseVariant);
  web->sendDataWs(responseVariant);
}