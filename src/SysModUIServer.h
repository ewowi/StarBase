#include "Module.h"

static std::vector<void(*)(JsonPair)> functions;

class SysModUIServer:public Module {

public:

  SysModUIServer() :Module("UI Server") {}; //constructor

  //serve index.htm
  void setup() {
    Module::setup();
    print->print("%s Setup:\n", name);

    success &= web->addURL("/", "/index.htm", "text/html");

    success &= web->processJSONURL("/json", processJSONURL);

    addGroup(name);
    addInput("Network name", "text");
    addInput("Password", "text");

    print->print(" %s\n", success?"success":"failed");
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

  void addGroup(const char *prompt, void(*fun)(JsonPair) = nullptr) {
    JsonObject props = addElement(prompt, "group", fun);
    print->print("addGroup %s\n", prompt);
  }

  JsonObject addInput(const char *prompt, const char *value, void(*fun)(JsonPair) = nullptr) {
    JsonObject props = addElement(prompt, "input", fun);
    props["value"] = value;
    print->print("addInput %s value %s\n", prompt, value);
    return props;
  }

  JsonObject addDisplay(const char *prompt, const char *value, void(*fun)(JsonPair) = nullptr) {
    JsonObject props = addElement(prompt, "display", fun);
    props["value"] = value;
    print->print("addDisplay %s value %s\n", prompt, value);
    return props;
  }

  JsonObject addCheckBox(const char *prompt, bool value, void(*fun)(JsonPair) = nullptr) {
    JsonObject props = addElement(prompt, "checkbox", fun);
    props["value"] = value;
    print->print("addCheckbox %s value %d\n", prompt, value);
    return props;
  }

  static JsonObject findObject(const char *prompt) { //static for processJSONURL
    JsonArray root = doc.as<JsonArray>();
    for(JsonObject object : root) {
      if (strcmp(object["prompt"], prompt) == 0)
        return object;
    }
    return JsonObject(); //null object
  }
  void setInput(const char *prompt, const char * value, void(*fun)(JsonPair) = nullptr) {
    JsonObject object = findObject(prompt);
    if (!object) object = addInput(prompt, value, fun);

    if (object) {
      if (object["value"] != value) {
        object["value"] = (char *)value; //copy
        web->sendDataWs(object);
        print->print("setInput %s = %s-> %s\n", prompt, object["value"].as<const char *>(), value);
      }
      else
        print->print("setInput %s = %s unchanged\n", prompt, value);
    }
    else
      print->print("could not create object %s with %s\n", prompt, value);
  }

  static void processJSONURL(JsonVariant &json) {
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
            if (!object["fun"].isNull()) {//isnull needed here!
              size_t funNr = object["fun"];
              functions[funNr](pair);
            }

            // web->docUpdated = true;
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

  void finishUI() { //tbd: automatic?
    web->sendDataWs(nullptr, true);
  }
};

static SysModUIServer *ui;