#pragma once //as also included in ModModel
#include "Module.h"

class SysModUIServer:public Module {

public:
  static std::vector<void(*)(const char *, JsonVariant)> functions;

  SysModUIServer() :Module("UI Server") {};

  //serve index.htm
  void setup() {
    Module::setup();

    print->print("%s Setup:\n", name);

    success &= web->addURL("/", "/index.htm", "text/html");

    success &= web->processJSONURL("/json", processJSONURL);

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();
  }

  JsonObject initGroup(JsonObject parent, const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(parent, prompt, "group", fun);
    if (object["value"].isNull() && value) object["value"] = value;
    return object;
  }

  JsonObject initInput(JsonObject parent, const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(parent, prompt, "input", fun);
    if (object["value"].isNull() && value) object["value"] = value;
    return object;
  }

  JsonObject initPassword(JsonObject parent, const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(parent, prompt, "password", fun);
    if (object["value"].isNull() && value) object["value"] = value;
    return object;
  }

  JsonObject initNumber(JsonObject parent, const char *prompt, int value, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(parent, prompt, "number", fun);
    if (object["value"].isNull()) object["value"] = value;
    return object;
  }

  JsonObject initDisplay(JsonObject parent, const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(parent, prompt, "display", fun);
    if (object["value"].isNull() && value) object["value"] = value;
    return object;
  }

  JsonObject initCheckBox(JsonObject parent, const char *prompt, bool value, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(parent, prompt, "checkbox", fun);
    if (object["value"].isNull()) object["value"] = value;
    return object;
  }

  JsonObject initButton(JsonObject parent, const char *prompt, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(parent, prompt, "button", fun);
    return object;
  }

  JsonObject initObject(JsonObject parent, const char *prompt, const char *type, void(*fun)(const char *, JsonVariant) = nullptr) {
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
      if (fun) {
        //if fun already in functions then reuse, otherwise add new fun in functions
        std::vector<void(*)(const char *, JsonVariant)>::iterator itr = find(functions.begin(), functions.end(), fun);
        if (itr!=functions.end()) //found
          object["fun"] = distance(functions.begin(), itr); //assign found function
        else { //not found
          functions.push_back(fun); //add new function
          object["fun"] = functions.size()-1;
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

        //call post function...
        if (!object["fun"].isNull()) {//isnull needed here!
          size_t funNr = object["fun"];
          functions[funNr](prompt, object["value"]);
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

  static JsonObject findObject(const char *prompt, JsonArray parent = JsonArray()) { //static for processJSONURL
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

  static void processJSONURL(JsonVariant &json) {
    if (json.is<JsonObject>()) //should be
    {
      for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
        const char * key = pair.key().c_str();
        JsonVariant value = pair.value();

        JsonObject object = findObject(key);
        if (!object.isNull())
        {
          if (strcmp(object["type"], "button") == 0 || object["value"] != value) { // if changed
            print->print("processJSONURL %s %s->%s\n", key, object["value"].as<String>(), value.as<String>());

            //assign new value
            if (value.is<const char *>())
              object["value"] = (char *)value.as<const char *>(); //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/)
            else if (value.is<bool>())
              object["value"] = value.as<bool>();
            else if (value.is<int>())
              object["value"] = value.as<int>();
            else {
              print->print("processJSONURL %s %s->%s not a supported type yet\n", key, object["value"].as<String>(), value.as<String>());
              // object["value"] = value;
            }

            //call post function...
            if (!object["fun"].isNull()) {//isnull needed here!
              size_t funNr = object["fun"];
              functions[funNr](key, object["value"]);
            }

            web->sendDataWs(object);
          }

        }
        else
          print->print("Object %s not found\n", key);
      }
    }
    else
      print->print("Json not object???\n");
  }

};

static SysModUIServer *ui;

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(const char *, JsonVariant)> SysModUIServer::functions;