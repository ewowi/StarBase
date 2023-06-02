#pragma once //as also included in ModModel
#include "Module.h"

static std::vector<void(*)(const char *, JsonVariant)> functions;

class SysModUIServer:public Module {

public:

  SysModUIServer() :Module("UI Server") {}; //constructor

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

  JsonObject initGroup(const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(prompt, "group", fun);
    if (object["value"].isNull() && value) object["value"] = value;
    return object;
  }

  JsonObject initInput(const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(prompt, "input", fun);
    if (object["value"].isNull() && value) object["value"] = value;
    return object;
  }

  JsonObject initNumber(const char *prompt, int value, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(prompt, "number", fun);
    if (object["value"].isNull()) object["value"] = value;
    return object;
  }

  JsonObject initDisplay(const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(prompt, "display", fun);
    if (object["value"].isNull() && value) object["value"] = value;
    return object;
  }

  JsonObject initCheckBox(const char *prompt, bool value, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(prompt, "checkbox", fun);
    if (object["value"].isNull()) object["value"] = value;
    return object;
  }

  JsonObject initButton(const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = initObject(prompt, "button", fun);
    if (object["value"].isNull()) object["value"] = value;
    return object;
  }

  JsonObject setInput(const char *prompt, const char * value) {
    JsonObject object = initObject(prompt, "input");
    if (object["value"].isNull() || object["value"] != value) {
      // print->print("  setInput changed %s %s\n", object["value"].as<String>(), value);
      object["value"] = (char *)value; ////(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
      web->sendDataWs(object);
    }
    return object;
  }

  JsonObject setDisplay(const char *prompt, const char * value) {
    JsonObject object = initObject(prompt, "display");
    if (object["value"].isNull() || object["value"] != value) {
      // print->print("  setDisplay changed %s %s\n", object["value"].as<String>(), value);
      object["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
      web->sendDataWs(object);
    }
    return object;
  }

  JsonObject setCheckBox(const char *prompt, bool value) {
    JsonObject object = initObject(prompt, "checkbox");
    if (object["value"].isNull() || object["value"] != value) {
      // print->print("  setInput changed %s %s\n", object["value"].as<String>(), value);
      object["value"] = value; ////(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
      web->sendDataWs(object);
    }
    return object;
  }

  JsonVariant getValue(const char *prompt) {
    JsonObject object = findObject(prompt);
    if (!object.isNull())
      return object["value"];
    else
      return JsonVariant();
  }

  JsonObject initObject(const char *prompt, const char *type, void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = findObject(prompt);

    //create new object
    if (object.isNull()) {
      print->print("initObject create new %s: %s\n", type, prompt);
      JsonArray root = model.as<JsonArray>();
      object = root.createNestedObject();
      object["prompt"] = prompt;
    }

    if (!object.isNull()) {
      //if existing also overwrite type and function
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

  static JsonObject findObject(const char *prompt) { //static for processJSONURL
    JsonArray root = model.as<JsonArray>();
    for(JsonObject object : root) {
      if (strcmp(object["prompt"], prompt) == 0)
        return object;
    }
    return JsonObject(); //null object
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
            // print->print("processJSONURL %s %s->%s\n", key, object["value"].as<String>(), value.as<String>());

            //assign new value
            if (value.is<const char *>())
              object["value"] = (char *)value.as<const char *>(); //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/)
            else if (value.is<bool>())
              object["value"] = value.as<bool>();
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