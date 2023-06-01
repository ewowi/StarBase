#pragma once //as also included in ModModel
#include "Module.h"

static std::vector<void(*)(const char *, JsonVariant)> functions;

class SysModUIServer:public Module {

public:

  StaticJsonDocument<1024> valueDoc;
  JsonVariant newValue;

  SysModUIServer() :Module("UI Server") {}; //constructor

  //serve index.htm
  void setup() {
    Module::setup();

    newValue = valueDoc.to<JsonVariant>();

    print->print("%s Setup:\n", name);

    success &= web->addURL("/", "/index.htm", "text/html");

    success &= web->processJSONURL("/json", processJSONURL);

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();
  }

  void defGroup(const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    newValue.set(value);
    defObject(prompt, "group", newValue, fun);
  }

  void defInput(const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    newValue.set(value);
    defObject(prompt, "input", newValue, fun);
  }

  void defNumber(const char *prompt, int value, void(*fun)(const char *, JsonVariant) = nullptr) {
    newValue.set(value);
    defObject(prompt, "number", newValue, fun);
  }

  void defDisplay(const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    newValue.set(value);
    defObject(prompt, "display", newValue, fun);
  }

  void defCheckBox(const char *prompt, bool value, void(*fun)(const char *, JsonVariant) = nullptr) {
    newValue.set(value);
    defObject(prompt, "checkbox", newValue, fun);
  }

  void defButton(const char *prompt, const char * value = nullptr, void(*fun)(const char *, JsonVariant) = nullptr) {
    newValue.set(value);
    defObject(prompt, "button", newValue, fun);
  }


  JsonObject defObject(const char *prompt, const char *type, JsonVariant value = JsonVariant(), void(*fun)(const char *, JsonVariant) = nullptr) {
    JsonObject object = findObject(prompt);

    //create new object
    if (object.isNull()) {
      print->print("setObject create new %s as %s = %s\n", prompt, type, value.as<String>());
      JsonArray root = model.as<JsonArray>();
      object = root.createNestedObject();
      object["prompt"] = prompt;
      object["value"] = value;
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
      print->print("setObject could not find or create object %s with %s\n", prompt, type);

    return object;
  }

  JsonObject setObject(const char *prompt, const char *type, JsonVariant value = JsonVariant()) {
    JsonObject object = findObject(prompt);

    //create new object
    if (object.isNull()) {
      print->print("setObject create new %s with %s\n", prompt, value.as<String>());
      JsonArray root = model.as<JsonArray>();
      object = root.createNestedObject();
      object["prompt"] = prompt;
      object["type"] = type;
    }

    if (!value.isNull()) compareAndAssign(object, value);

    return object;
  }


  void setInput(const char *prompt, const char * value) {
    newValue.set(value);
    setObject(prompt, "input", newValue);
  }

  void setDisplay(const char *prompt, const char * value) {
    newValue.set(value);
    setObject(prompt, "display", newValue);
  }

  void setCheckBox(const char *prompt, bool value, void(*fun)(const char *, JsonVariant) = nullptr) {
    newValue.set(value);
    setObject(prompt, "checkbox", newValue);
  }

  void setButton(const char *prompt, bool value, void(*fun)(const char *, JsonVariant) = nullptr) {
    newValue.set(value);
    setObject(prompt, "button", newValue);
  }

  static JsonObject findObject(const char *prompt) { //static for processJSONURL
    JsonArray root = model.as<JsonArray>();
    for(JsonObject object : root) {
      if (strcmp(object["prompt"], prompt) == 0)
        return object;
    }
    return JsonObject(); //null object
  }

  static void compareAndAssign(JsonObject object, JsonVariant value, bool doFun = false) {
    if (strcmp(object["type"], "button") == 0 || object["value"] != value) { // if changed
      const char * key = object["prompt"];
      // if (strcmp(object["prompt"], "UpTime") == 0)
      // print->print("compareAndAssign %s %s->%s\n", key, object["value"].as<String>(), value.as<String>());

      //assign new value
      if (value.is<const char *>())
        object["value"] = (char *)value.as<const char *>(); //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/)
      else if (value.is<bool>())
        object["value"] = value.as<bool>();
      else {
        print->print("compareAndAssign %s %s->%s not a supported type yet\n", key, object["value"].as<String>(), value.as<String>());
        // object["value"] = value;
      }

      //call post function...
      if (doFun && !object["fun"].isNull()) {//isnull needed here!
        size_t funNr = object["fun"];
        functions[funNr](key, object["value"]);
      }

      web->sendDataWs(object);
    }
  }

  //not used yet
  JsonVariant getValue(const char *prompt) {
    JsonObject object = findObject(prompt);
    if (!object.isNull())
      return object["value"];
    else
      return JsonVariant();
  }

  static void processJSONURL(JsonVariant &json) {
    if (json.is<JsonObject>()) //should be
    {
      for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
        const char * key = pair.key().c_str();
        JsonVariant value = pair.value();

        JsonObject object = findObject(key);
        if (!object.isNull())
          compareAndAssign(object, value, true);
        else
          print->print("Object %s not found\n", key);
      }
    }
    else
      print->print("Json not object???\n");
  }

  // void finishUI() { //tbd: automatic?
  //   web->sendDataWs(nullptr, true);
  // }

};

static SysModUIServer *ui;