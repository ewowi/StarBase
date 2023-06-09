#pragma once //as also included in ModModel
#include "Module.h"

class SysModUIServer:public Module {

public:
  static std::vector<void(*)(const char *, JsonVariant)> chFunctions;
  static std::vector<JsonVariant(*)(const char *)> uiFunctions;

  SysModUIServer() :Module("UI Server") {};

  //serve index.htm
  void setup() {
    Module::setup();

    print->print("%s Setup:\n", name);

    success &= web->addURL("/", "/index.htm", "text/html");

    success &= web->processJsonUrl("/json", processJson); //for http requests

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();
  }

  JsonObject initGroup(JsonObject parent, const char *prompt, const char * value = nullptr, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = initObject(parent, prompt, "group", chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(prompt, object["value"]);
    return object;
  }

  JsonObject initInput(JsonObject parent, const char *prompt, const char * value = nullptr, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = initObject(parent, prompt, "input", chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(prompt, object["value"]);
    return object;
  }

  JsonObject initPassword(JsonObject parent, const char *prompt, const char * value = nullptr, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = initObject(parent, prompt, "password", chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(prompt, object["value"]);
    return object;
  }

  JsonObject initNumber(JsonObject parent, const char *prompt, int value, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = initObject(parent, prompt, "number", chFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(prompt, object["value"]);
    return object;
  }

  JsonObject initDisplay(JsonObject parent, const char *prompt, const char * value = nullptr, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = initObject(parent, prompt, "display", chFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(prompt, object["value"]);
    return object;
  }

  JsonObject initCheckBox(JsonObject parent, const char *prompt, bool value, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = initObject(parent, prompt, "checkbox", chFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(prompt, object["value"]);
    return object;
  }

  JsonObject initButton(JsonObject parent, const char *prompt, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = initObject(parent, prompt, "button", chFun);
    //no call of fun for buttons!!! 
    return object;
  }

  JsonObject initDropdown(JsonObject parent, const char *prompt, uint8_t value, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = initObject(parent, prompt, "dropdown", chFun, uiFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(prompt, object["value"]);
    return object;
  }

  JsonObject initObject(JsonObject parent, const char *prompt, const char *type, void(*chFun)(const char *, JsonVariant) = nullptr, JsonVariant(*uiFun)(const char *) = nullptr) {
    JsonObject object = findObject(prompt);

    //create new object
    if (object.isNull()) {
      print->print("initObject create new %s: %s\n", type, prompt);
      if (parent.isNull()) {
        JsonArray root = model.as<JsonArray>();
        object = root.createNestedObject();
      } else {
        if (parent["n"].isNull()) parent.createNestedArray("n");
        object = parent["n"].createNestedObject();
        // serializeJson(model, Serial);print->print("\n");
      }
      object["prompt"] = prompt;
    }
    else
      print->print("Object %s already defined\n", prompt);

    if (!object.isNull()) {
      object["type"] = type;
      if (chFun) {
        //if fun already in chFunctions then reuse, otherwise add new fun in chFunctions
        std::vector<void(*)(const char *, JsonVariant)>::iterator itr = find(chFunctions.begin(), chFunctions.end(), chFun);
        if (itr!=chFunctions.end()) //found
          object["chFun"] = distance(chFunctions.begin(), itr); //assign found function
        else { //not found
          chFunctions.push_back(chFun); //add new function
          object["chFun"] = chFunctions.size()-1;
        }
      }
      if (uiFun) {
        //if fun already in uiFunctions then reuse, otherwise add new fun in uiFunctions
        std::vector<JsonVariant(*)(const char *)>::iterator itr = find(uiFunctions.begin(), uiFunctions.end(), uiFun);
        if (itr!=uiFunctions.end()) //found
          object["uiFun"] = distance(uiFunctions.begin(), itr); //assign found function
        else { //not found
          uiFunctions.push_back(uiFun); //add new function
          object["uiFun"] = uiFunctions.size()-1;
        }
      }
    }
    else
      print->print("initObject could not find or create object %s with %s\n", prompt, type);

    return object;
  }

  JsonObject setValue(const char *prompt, const char * value) {
    JsonObject object = findObject(prompt);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("  setValue changed %s %s\n", object["value"].as<String>(), value);
        object["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)

        //call change function...
        if (!object["chFun"].isNull()) {//isnull needed here!
          size_t funNr = object["chFun"];
          chFunctions[funNr](prompt, object["value"]);
        }

        web->sendDataWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", prompt);

    return object;
  }

  JsonObject setValue(const char *prompt, bool value) {
    JsonObject object = findObject(prompt);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("  setInput changed %s %s\n", object["value"].as<String>(), value);
        object["value"] = value;
        web->sendDataWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", prompt);

    return object;
  }

  //Argument list
  JsonObject setValueV(const char *prompt, const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);

    va_end(args);

    return setValue(prompt, value);
  }

  //Argument list and print
  JsonObject setValueP(const char *prompt, const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);
    print->print("%s\n", value);

    va_end(args);

    return setValue(prompt, value);
  }

  const char * getValue(const char *prompt) {
    JsonObject object = findObject(prompt);
    if (!object.isNull())
      return object["value"];
    else
      return nullptr;
  }

  static JsonObject findObject(const char *prompt, JsonArray parent = JsonArray()) { //static for processJson
    JsonArray root;
    // print ->print("findObject %s %s\n", prompt, parent.isNull()?"root":"n");
    if (parent.isNull()) {
      root = model.as<JsonArray>();
    }
    else {
      root = parent;
    }
    JsonObject foundObject;
    for(JsonObject object : root) {
      if (foundObject.isNull()) {
        if (strcmp(object["prompt"], prompt) == 0)
          foundObject = object;
        else if (!object["n"].isNull())
          foundObject = findObject(prompt, object["n"]);
      }
    }
    return foundObject;
  }

  static JsonVariant processJson(JsonVariant &json) { //static for processJsonUrl
    if (json.is<JsonObject>()) //should be
    {
      for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
        const char * key = pair.key().c_str();
        JsonVariant value = pair.value();

        //special commands
        if (strcmp(key, "uiFun")==0) {
            //find the dropdown object and collect it's options...
            JsonObject object = findObject(value); //value is the prompt
            if (!object.isNull()) {
              //call ui function...
              if (!object["uiFun"].isNull()) {//isnull needed here!
                size_t funNr = object["uiFun"];
                JsonVariant uiStuff = uiFunctions[funNr](key);
                char resStr[50]; 
                serializeJson(uiStuff, resStr);
                serializeJson(uiStuff, Serial);

                print->print("special command %s %s = %s: %s\n", key, object["prompt"].as<String>(), value.as<String>(), resStr);
                return uiStuff; //tbd: if multiple add in array and return array
              }
            }
            else
              print->print("special command %s object %s not found\n", key, value.as<String>());
        }
        else {
          JsonObject object = findObject(key);
          if (!object.isNull())
          {
            if (strcmp(object["type"], "button") == 0 || object["value"] != value) { // if changed
              print->print("processJson %s %s->%s\n", key, object["value"].as<String>(), value.as<String>());

              //assign new value
              if (value.is<const char *>())
                object["value"] = (char *)value.as<const char *>(); //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/)
              else if (value.is<bool>())
                object["value"] = value.as<bool>();
              else if (value.is<int>())
                object["value"] = value.as<int>();
              else {
                print->print("processJson %s %s->%s not a supported type yet\n", key, object["value"].as<String>(), value.as<String>());
                // object["value"] = value;
              }

              //call change function...
              if (!object["chFun"].isNull()) {//isnull needed here!
                size_t funNr = object["chFun"];
                chFunctions[funNr](key, object["value"]);
              }

              web->sendDataWs(object);
            }

          }
          else
            print->print("Object %s not found\n", key);
        }
      } //for json pairs
    }
    else
      print->print("Json not object???\n");
    return JsonVariant();
  }

};

static SysModUIServer *ui;

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(const char *, JsonVariant)> SysModUIServer::chFunctions;
std::vector<JsonVariant(*)(const char *)> SysModUIServer::uiFunctions;