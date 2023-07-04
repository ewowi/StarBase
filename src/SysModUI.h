#pragma once //as also included in ModModel
#include "Module.h"
#include "SysModWeb.h"


class SysModUI:public Module {

  StaticJsonDocument<2048> responseDoc;

public:
  static std::vector<void(*)(JsonObject object)> uiFunctions;

  SysModUI() :Module("UI") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);
    success &= web->addURL("/", "/index.htm", "text/html");
    // success &= web->addURL("/index.js", "/index.js", "text/javascript");
    // success &= web->addURL("/index.css", "/index.css", "text/css");

    success &= web->setupJsonHandlers("/json", processJson);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //serve index.htm
  void setup() {
    Module::setup();
  }

  void loop() {
    // Module::loop();
  }

  JsonObject initGroup(JsonObject parent, const char *id, const char * value = nullptr, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "group", uiFun, chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initMany(JsonObject parent, const char *id, const char * value = nullptr, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "many", uiFun, chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initInput(JsonObject parent, const char *id, const char * value = nullptr, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "input", uiFun, chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initPassword(JsonObject parent, const char *id, const char * value = nullptr, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "password", uiFun, chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initNumber(JsonObject parent, const char *id, int value, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "number", uiFun, chFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initDisplay(JsonObject parent, const char *id, const char * value = nullptr, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "display", uiFun, chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initCheckBox(JsonObject parent, const char *id, bool value, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "checkbox", uiFun, chFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initButton(JsonObject parent, const char *id, const char * value = nullptr, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "button", uiFun, chFun);
    if (object["value"].isNull()) object["value"] = value;
    //no call of fun for buttons!!! 
    // if (chFun) chFun(object);
    return object;
  }

  JsonObject initDropdown(JsonObject parent, const char *id, uint8_t value, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = initObject(parent, id, "dropdown", uiFun, chFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initObject(JsonObject parent, const char *id, const char *type, void(*uiFun)(JsonObject) = nullptr, void(*chFun)(JsonObject object) = nullptr) {
    JsonObject object = findObject(id);

    //create new object
    if (object.isNull()) {
      print->print("initObject create new %s: %s\n", type, id);
      if (parent.isNull()) {
        JsonArray root = model.as<JsonArray>();
        object = root.createNestedObject();
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
      if (uiFun) {
        //if fun already in uiFunctions then reuse, otherwise add new fun in uiFunctions
        std::vector<void(*)(JsonObject object)>::iterator itr = find(uiFunctions.begin(), uiFunctions.end(), uiFun);
        if (itr!=uiFunctions.end()) //found
          object["uiFun"] = distance(uiFunctions.begin(), itr); //assign found function
        else { //not found
          uiFunctions.push_back(uiFun); //add new function
          object["uiFun"] = uiFunctions.size()-1;
        }
      }
      if (chFun) {
        //if fun already in uiFunctions then reuse, otherwise add new fun in uiFunctions
        std::vector<void(*)(JsonObject object)>::iterator itr = find(uiFunctions.begin(), uiFunctions.end(), chFun);
        if (itr!=uiFunctions.end()) //found
          object["chFun"] = distance(uiFunctions.begin(), itr); //assign found function
        else { //not found
          uiFunctions.push_back(chFun); //add new function
          object["chFun"] = uiFunctions.size()-1;
        }
      }
    }
    else
      print->print("initObject could not find or create object %s with %s\n", id, type);

    return object;
  }

  //setValue char
  static JsonObject setValue(const char *id, const char * value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
        object["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", id);
    return object;
  }

  //setValue int
  static JsonObject setValue(const char *id, int value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
        object["value"] = value;
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", id);

    return object;
  }

  //setValue bool
  static JsonObject setValue(const char *id, bool value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value?"true":"false");
        object["value"] = value;
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", id);
    return object;
  }

  static void setChFunAndWs(JsonObject object) {

    if (!object["chFun"].isNull()) {//isnull needed here!
      size_t funNr = object["chFun"];
      uiFunctions[funNr](object);
    }

    responseDoc.clear(); //needed for deserializeJson?
    if (object["value"].is<int>())
      web->addResponseInt(object, "value", object["value"].as<int>());
    if (object["value"].is<bool>())
      web->addResponseBool(object, "value", object["value"].as<bool>());
    else
      web->addResponse(object, "value", object["value"]);
    char resStr[200]; 
    serializeJson(responseDoc, resStr);
    // print->print("setChFunAndWs send response %s\n", resStr);
    web->sendDataWs(responseDoc.as<JsonVariant>());
  }

  //Set value with argument list
  JsonObject setValueV(const char *id, const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);

    va_end(args);

    return setValue(id, value);
  }

  //Set value with argument list and print
  JsonObject setValueP(const char *id, const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);
    print->print("%s\n", value);

    va_end(args);

    return setValue(id, value);
  }

  JsonVariant getValue(const char *id) {
    JsonObject object = findObject(id);
    if (!object.isNull())
      return object["value"];
    else {
      print->print("Value of %s does not exist!!\n", id);
      return JsonVariant();
    }
  }

  static JsonObject findObject(const char *id, JsonArray parent = JsonArray()) { //static for processJson
    JsonArray root;
    // print ->print("findObject %s %s\n", id, parent.isNull()?"root":"n");
    if (parent.isNull()) {
      root = model.as<JsonArray>();
    }
    else {
      root = parent;
    }
    JsonObject foundObject;
    for(JsonObject object : root) {
      if (foundObject.isNull()) {
        if (strcmp(object["id"], id) == 0)
          foundObject = object;
        else if (!object["n"].isNull())
          foundObject = findObject(id, object["n"]);
      }
    }
    return foundObject;
  }

  static const char * processJson(JsonVariant &json) { //static for setupJsonHandlers
    if (json.is<JsonObject>()) //should be
    {
      for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
        const char * key = pair.key().c_str();
        JsonVariant value = pair.value();

        // commands
        if (strcmp(key, "uiFun")==0) {
          //find the dropdown object and collect it's options...
          JsonObject object = findObject(value); //value is the id
          if (!object.isNull()) {
            //call ui function...
            if (!object["uiFun"].isNull()) {//isnull needed here!
              size_t funNr = object["uiFun"];
              uiFunctions[funNr](object);
              if (object["type"] == "dropdown")
                web->addResponseInt(object, "value", object["value"]); //temp assume int only
              char resStr[200]; 
              serializeJson(responseDoc, resStr);

              print->print("command %s %s: %s\n", key, object["id"].as<const char *>(), resStr);
            }
          }
          else
            print->print("command %s object %s not found\n", key, value.as<String>().c_str());
        } else {
          if (!value.is<JsonObject>()) { //no objects (inserted by uiFun responses)
            JsonObject object = findObject(key);
            if (!object.isNull())
            {
              if (object["value"] != value) { // if changed
                print->print("processJson %s %s->%s\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());

                //set new value
                if (value.is<const char *>())
                  setValue(key, value.as<const char *>());
                else if (value.is<bool>())
                  setValue(key, value.as<bool>());
                else if (value.is<int>())
                  setValue(key, value.as<int>());
                else {
                  print->print("processJson %s %s->%s not a supported type yet\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());
                }
              }
              else if (strcmp(object["type"], "button") == 0)
                setChFunAndWs(object);
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

};

static SysModUI *ui;

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(JsonObject object)> SysModUI::uiFunctions;