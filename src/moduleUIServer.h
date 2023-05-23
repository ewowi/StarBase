#include "module.h"

static void(*func)(String, String); //tbd: muultiple funcs

class ModuleUIServer:public Module {

public:

  ModuleUIServer() :Module("UIServer") {}; //constructor

  //serve index.htm
  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    success &= web->addURL("/", "/index.htm", "text/html");

    success &= web->processURL("/json", processJSON);

    print->print(" %s\n", success?"success":"failed");
    JsonObject root = doc.to<JsonObject>(); //create
  }

  void loop(){
    // Module::loop();
  }

  static void processJSON(AsyncWebServerRequest *request) {
    // probably replace by AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/json", [](AsyncWebServerRequest *request) {});
    // and move to WebServer?

    String id;
    String value;
    if (request->hasParam("id") && request->hasParam("value")) {
      id = request->getParam("id")->value();
      value = request->getParam("value")->value();
    }
    else {
      id = "No message sent";
      value = "No message sent";
    }
    print->print("%s := %s %s\n", id, value, func?"y":"n");
    //call post function...
    if (func) func(id, value);
    request->send(200, "text/plain", "OK");
  }

  void addCheckBox(const char *prompt, bool *value, void(*funcy)(String, String) = nullptr) {
    JsonObject root = doc.as<JsonObject>();

    JsonArray body;
    if (root["body"].isNull())
      body = root.createNestedArray("body");
    else 
      body = root["body"];

    JsonObject props = body.createNestedObject();
    props["prompt"] = prompt;
    props["value"] = *value;
    // props["valueptr"] = value;
    props["type"] = "checkbox";
    // props["func"] = funcy;

    if (funcy) func = funcy;
  };

  void finishUI() {
    web->sendDataWs(nullptr, true);
  }
};

static ModuleUIServer *ui;