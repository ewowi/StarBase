/*
   @title     StarMod
   @file      SysModUI.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModModel.h"

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<UFun> SysModUI::uFunctions;
std::vector<CFun> SysModUI::cFunctions;
std::vector<VarLoop> SysModUI::loopFunctions;
int SysModUI::varCounter = 1; //start with 1 so it can be negative, see var["o"]
bool SysModUI::stageVarChanged = false;

SysModUI::SysModUI() :SysModule("UI") {
};

//serve index.htm
void SysModUI::setup() {
  SysModule::setup();

  parentVar = initSysMod(parentVar, name);
  if (parentVar["o"] > -1000) parentVar["o"] = -4100; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

  JsonObject tableVar = initTable(parentVar, "vlTbl", nullptr, true, [](JsonObject var) { //uiFun ro true: no update and delete
    web->addResponse(var["id"], "label", "Variable loops");
    web->addResponse(var["id"], "comment", "Loops initiated by a variable");
    JsonArray rows = web->addResponseA(var["id"], "value"); //overwrite the value

    for (auto varLoop = begin (loopFunctions); varLoop != end (loopFunctions); ++varLoop) {
      JsonArray row = rows.createNestedArray();
      row.add(varLoop->var["id"]);
      row.add(varLoop->counter);
    }
  });
  initText(tableVar, "vlVar", nullptr, UINT8_MAX, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Name");
  });
  initNumber(tableVar, "vlLoopps", UINT16_MAX, 0, 999, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Loops p s");
  });
}

void SysModUI::loop() {
  // SysModule::loop();

  for (auto varLoop = begin (loopFunctions); varLoop != end (loopFunctions); ++varLoop) {
    if (millis() - varLoop->lastMillis >= varLoop->var["interval"].as<int>()) {
      varLoop->lastMillis = millis();

      varLoop->loopFun(varLoop->var, 1); //rowNr..

      varLoop->counter++;
      // USER_PRINTF("%s %u %u %d %d\n", varLoop->var["id"].as<const char *>(), varLoop->lastMillis, millis(), varLoop->interval, varLoop->counter);
    }
  }
}

void SysModUI::loop1s() {
  //if something changed in vloops
  uint8_t rowNr = 0;
  for (auto varLoop = begin (loopFunctions); varLoop != end (loopFunctions); ++varLoop) {
    mdl->setValue("vlLoopps", varLoop->counter, rowNr++);
    varLoop->counter = 0;
  }
}

JsonObject SysModUI::initVar(JsonObject parent, const char * id, const char * type, bool readOnly, UFun uiFun, CFun chFun, CFun loopFun) {
  JsonObject var = mdl->findVar(id); //sets the existing modelParentVar
  const char * modelParentId = mdl->modelParentVar["id"];
  const char * parentId = parent["id"];

  bool differentParents = modelParentId != nullptr && parentId != nullptr && strcmp(modelParentId, parentId) != 0;
  //!mdl->modelParentVar.isNull() && !parent.isNull() && mdl->modelParentVar["id"] != parent["id"];
  if (differentParents) {
    USER_PRINTF("initVar parents not equal %s: %s != %s\n", id, modelParentId, parentId);
  }

  //create new var
  if (differentParents || var.isNull()) {
    USER_PRINTF("initVar create new %s var: %s->%s\n", type, parentId, id);
    if (parent.isNull()) {
      JsonArray vars = mdl->model->as<JsonArray>();
      var = vars.createNestedObject();
    } else {
      if (parent["n"].isNull()) parent.createNestedArray("n"); //if parent exist and no "n" array, create it
      var = parent["n"].createNestedObject();
      // serializeJson(model, Serial);Serial.println();
    }
    var["id"] = JsonString(id, JsonString::Copied);
  }
  // else {
  //   USER_PRINTF("initVar Var %s->%s already defined\n", modelParentId, id);
  // }

  if (!var.isNull()) {
    if (var["type"].isNull() || strcmp(var["type"].as<const char *>(),  type) != 0 ) {
      var["type"] = JsonString(type, JsonString::Copied);
      print->printJson("initVar set type", var);
    }

    if (var["ro"] != readOnly) var["ro"] = readOnly;

    //set order. make order negative to check if not obsolete, see cleanUpModel
    if (var["o"] >= 1000) //predefined! (modules)
      var["o"] = -var["o"].as<int>(); //leave the order as is
    else {
      if (parent["o"].as<int>() >= 0) // if checks on the parent already done so vars added later, e.g. controls, will be autochecked
        var["o"] = varCounter++; //redefine order
      else
        var["o"] = -varCounter++; //redefine order
    }


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
        uFunctions.push_back(uiFun); //add new function
        var["uiFun"] = uFunctions.size()-1;
      // }
    }

    //if chFun, add it to the list
    if (chFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      // std::vector<UCFun>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), chFun);
      // if (itr!=ucFunctions.end()) //found
      //   var["chFun"] = distance(ucFunctions.begin(), itr); //assign found function
      // else { //not found
        cFunctions.push_back(chFun); //add new function
        var["chFun"] = cFunctions.size()-1;
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
      // USER_PRINTF("iObject loopFun %s %u %u %d %d\n", var["id"].as<const char *>());
    }
  }
  else
    USER_PRINTF("initVar could not find or create var %s with %s\n", id, type);

  return var;
}

void SysModUI::processJson(JsonVariant json) {
  if (json.is<JsonObject>()) //should be
  {
     //uiFun adds object elements to json which would be processed in the for loop. So we freeze the original pairs in a vector and loop on this
    std::vector<JsonPair> pairs;
    for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
      pairs.push_back(pair);
    }

    for (JsonPair pair : pairs) { //iterate json elements
      const char * key = pair.key().c_str();
      JsonVariant value = pair.value();

      // commands
      if (pair.key() == "view" || pair.key() == "canvasData" || pair.key() == "theme") { //save the chosen view in System (see index.js)
        JsonObject var = mdl->findVar("System");
        USER_PRINTF("processJson %s v:%s n: %d s:%s\n", pair.key().c_str(), pair.value().as<String>().c_str(), var.isNull(), var["id"].as<const char *>());
        var[JsonString(key, JsonString::Copied)] = JsonString(value, JsonString::Copied);
        // json.remove(key); //key should stay as all clients use this to perform the changeHTML action
      }
      else if (pair.key() == "addRow" || pair.key() == "delRow") {
        USER_PRINTF("processJson %s - %s\n", key, value.as<String>().c_str());
        if (value.is<JsonObject>()) {
          JsonObject command = value.as<JsonObject>();
          JsonObject var = mdl->findVar(command["id"]);
          // var["value"] = pair.key(); //store the action as variable workaround?
          mdl->callChFunAndWs(var, command["rowNr"].as<int>(), pair.key().c_str()); //let chFun deal with it
        }
        json.remove(key); //key processed we don't need the key in the response
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
                if (funNr < uFunctions.size())
                  uFunctions[funNr](var);
                else    
                  USER_PRINTF("processJson function nr %s outside bounds %d >= %d\n", var["id"].as<const char *>(), funNr, uFunctions.size());

                //if select var, send value back
                // if (var["type"] == "select") {
                //   if (var["value"].is<JsonArray>()) //for tables
                //     web->addResponseArray(var["id"], "value", var["value"]);
                //   else
                //     web->addResponseI(var["id"], "value", var["value"]);
                // }

                // print->printJson("PJ Command", responseDoc);
              }
            }
            else
              USER_PRINTF("processJson Command %s var %s not found\n", key, value2.as<String>().c_str());
          }
        } else
          USER_PRINTF("processJson value not array? %s %s\n", key, value.as<String>().c_str());
        json.remove(key); //key processed we don't need the key in the response
      } 
      else if (!value.isNull()) { // {"varid": {"value":value}} or {"varid": value}

        JsonVariant val;
        if (value["value"].isNull()) // if no explicit value field (e.g. jsonHandler)
          val = value;
        else
          val = value["value"]; //use the value field

        //check if we deal with multiple rows (from table type)
        char * rowNrC = strtok((char *)key, "#");
        if (rowNrC != NULL ) {
          key = rowNrC;
          rowNrC = strtok(NULL, " ");
        }
        uint8_t rowNr = rowNrC?atoi(rowNrC):UINT8_MAX;

        JsonObject var = mdl->findVar(key);

        USER_PRINTF("processJson %s[%d] (%s == %s ? %d)\n", key, rowNr, var["value"].as<String>().c_str(), val.as<String>().c_str(), var["value"] == val);

        if (!var.isNull())
        {
          //a button never sets the value
          if (var["type"] == "button") { //button always
            mdl->callChFunAndWs(var, rowNr, val); //bypass var["value"] 
            print->printJson("button", json);
          }
          else {
            USER_PRINTF("processJson %s[%d] %s->%s\n", key, rowNr, var["value"].as<String>().c_str(), value.as<String>().c_str());
            if (val.is<const char *>())
              mdl->setValue(var, JsonString(val, JsonString::Copied), rowNr);
            else
              mdl->setValue(var, val.as<JsonVariant>(), rowNr);
          }
          // json.remove(key); //key / var["id"] processed we don't need the key in the response
        }
        else
          USER_PRINTF("dev Object %s[%d] not found\n", key, rowNr);
      } 
      else {
        USER_PRINTF("dev processJson command not recognized k:%s v:%s\n", key, value.as<String>().c_str());
      }
    } //for json pairs
  }
}

void SysModUI::processUiFun(const char * id) {
  JsonObject responseObject = web->getResponseDoc()->to<JsonObject>();

  JsonArray array = responseObject.createNestedArray("uiFun");
  array.add(id);
  processJson(responseObject); //this calls uiFun command if the var with id provided
  //this also updates uiFun stuff - not needed!

  web->sendDataWs(responseObject); //not needed?
}