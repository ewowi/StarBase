#include "module.h"

static std::vector<void(*)(JsonPair)> functions; 

class ModuleUIServer:public Module {

public:

  ModuleUIServer() :Module("UIServer") {}; //constructor

  //serve index.htm
  void setup() {
    Module::setup();
    print->print("%s Setup:\n", name);

    success &= web->addURL("/", "/index.htm", "text/html");

    success &= web->processJSONURL("/json", processJSONURL);

    print->print(" %s\n", success?"success":"failed");
    JsonArray root = doc.to<JsonArray>(); //create
  }

  void loop(){
    // Module::loop();
  }

  JsonObject addElement(const char *prompt, const char *type, void(*fun)(JsonPair) = nullptr) {
    JsonArray root = doc.as<JsonArray>();

    JsonObject props = root.createNestedObject();
    props["prompt"] = prompt;
    props["type"] = type;
    if (fun) { 
      functions.push_back(fun);
      props["fun"] = functions.size()-1;
    }

    return props;
  }

  void addInput(const char *prompt, const char *value, void(*fun)(JsonPair) = nullptr) {
    JsonObject props = addElement(prompt, "input", fun);
    props["value"] = value;
    print->print("addInput %s value %s\n", prompt, value);
  }

  void addCheckBox(const char *prompt, bool value, void(*fun)(JsonPair) = nullptr) {
    JsonObject props = addElement(prompt, "checkbox", fun);
    props["value"] = value;
    print->print("addCheckbox %s value %d\n", prompt, value);
  }

  static void processJSONURL(AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) //should be
    {
      for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
        const char * key = pair.key().c_str();
        JsonVariant value = pair.value();

        JsonObject object = findObject(key);
        if (object) {
          if (object["value"] != value) { // if changed
            print->print("processJSONURL %s value %s->%s\n", key, object["value"].as<String>(), value.as<String>());

            //assign new value
            if (strcmp(object["type"], "input") == 0 && value.is<const char *>())
              object["value"] = (char *)value.as<const char *>(); //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/)
            else if (strcmp(object["type"], "checkbox") == 0 && value.is<bool>())
              object["value"] = value;
            else
              print->print("Object %s type %s unknown or value %s not matching type \n", key, object["type"].as<const char *>(), value.as<const char *>());

            //call post function...
            if (!object["fun"].isNull()) {
              size_t funNr = object["fun"];
              functions[funNr](pair);
            }

            web->docUpdated = true;
          }
        }
        else
          print->print("Object %s not found\n", key);
      }
    }
    else
      print->print("Json not object???\n");

    request->send(200, "text/plain", "OK");
  }

  static JsonObject findObject(const char *id) {
    JsonArray root = doc.as<JsonArray>();
    for(JsonObject object : root) {
      if (strcmp(object["prompt"], id) == 0)
        return object;
    }
    return JsonObject(); //null object
  }

  void finishUI() { //tbd: automatic?
    web->sendDataWs(nullptr, true);
  }
};

static ModuleUIServer *ui;