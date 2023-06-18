#pragma once //as also included in ModModel
#include "Module.h"

class SysModUI:public Module {

public:
  static std::vector<void(*)(JsonObject object)> chFunctions;
  static std::vector<JsonVariant(*)(JsonObject object)> uiFunctions;

  SysModUI() :Module("UI Server") {};

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

  JsonObject initGroup(JsonObject parent, const char *prompt, const char * value = nullptr, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
    JsonObject object = initObject(parent, prompt, "group", chFun, uiFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initInput(JsonObject parent, const char *prompt, const char * value = nullptr, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
    JsonObject object = initObject(parent, prompt, "input", chFun, uiFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initPassword(JsonObject parent, const char *prompt, const char * value = nullptr, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
    JsonObject object = initObject(parent, prompt, "password", chFun, uiFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initNumber(JsonObject parent, const char *prompt, int value, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
    JsonObject object = initObject(parent, prompt, "number", chFun, uiFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initDisplay(JsonObject parent, const char *prompt, const char * value = nullptr, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
    JsonObject object = initObject(parent, prompt, "display", chFun, uiFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initCheckBox(JsonObject parent, const char *prompt, bool value, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
    JsonObject object = initObject(parent, prompt, "checkbox", chFun, uiFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initButton(JsonObject parent, const char *prompt, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
    JsonObject object = initObject(parent, prompt, "button", chFun, uiFun);
    //no call of fun for buttons!!! 
    return object;
  }

  JsonObject initDropdown(JsonObject parent, const char *prompt, uint8_t value, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
    JsonObject object = initObject(parent, prompt, "dropdown", chFun, uiFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initObject(JsonObject parent, const char *prompt, const char *type, void(*chFun)(JsonObject object) = nullptr, JsonVariant(*uiFun)(JsonObject) = nullptr) {
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
        std::vector<void(*)(JsonObject object)>::iterator itr = find(chFunctions.begin(), chFunctions.end(), chFun);
        if (itr!=chFunctions.end()) //found
          object["chFun"] = distance(chFunctions.begin(), itr); //assign found function
        else { //not found
          chFunctions.push_back(chFun); //add new function
          object["chFun"] = chFunctions.size()-1;
        }
      }
      if (uiFun) {
        //if fun already in uiFunctions then reuse, otherwise add new fun in uiFunctions
        std::vector<JsonVariant(*)(JsonObject object)>::iterator itr = find(uiFunctions.begin(), uiFunctions.end(), uiFun);
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

  //setValue char
  static JsonObject setValue(const char *prompt, const char * value) {
    JsonObject object = findObject(prompt);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("  setValue changed %s %s\n", object["value"].as<String>(), value);
        object["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", prompt);
    return object;
  }

  //setValue int
  static JsonObject setValue(const char *prompt, int value) {
    JsonObject object = findObject(prompt);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("  setInput changed %s %s\n", object["value"].as<String>(), value);
        object["value"] = value;
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", prompt);

    return object;
  }

  //setValue bool
  static JsonObject setValue(const char *prompt, bool value) {
    JsonObject object = findObject(prompt);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("  setInput changed %s %s\n", object["value"].as<String>(), value);
        object["value"] = value;
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", prompt);
    return object;
  }

  static void setChFunAndWs(JsonObject object) {

    if (!object["chFun"].isNull()) {//isnull needed here!
      size_t funNr = object["chFun"];
      chFunctions[funNr](object);
    }

    web->sendDataWs(object);
  }

  //Set value with argument list
  JsonObject setValueV(const char *prompt, const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);

    va_end(args);

    return setValue(prompt, value);
  }

  //Set value with argument list and print
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

  JsonVariant getValue(const char *prompt) {
    JsonObject object = findObject(prompt);
    if (!object.isNull())
      return object["value"];
    else
      return JsonVariant();
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
                JsonVariant uiStuff = uiFunctions[funNr](object);
                if (object["type"] == "dropdown") 
                  uiStuff["default"] = object["value"];
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
            if (object["value"] != value) { // if changed
              print->print("processJson %s %s->%s\n", key, object["value"].as<String>(), value.as<String>());

              //set new value
              if (value.is<const char *>())
                setValue(key, value.as<const char *>());
              else if (value.is<bool>())
                setValue(key, value.as<bool>());
              else if (value.is<int>())
                setValue(key, value.as<int>());
              else {
                print->print("processJson %s %s->%s not a supported type yet\n", key, object["value"].as<String>(), value.as<String>());
              }
            }
            else if (strcmp(object["type"], "button") == 0)
              setChFunAndWs(object);
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

static SysModUI *ui;

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(JsonObject object)> SysModUI::chFunctions;
std::vector<JsonVariant(*)(JsonObject object)> SysModUI::uiFunctions;