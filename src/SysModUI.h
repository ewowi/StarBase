#pragma once
#include <vector>
#include "ArduinoJson.h"
#include "module.h"


typedef void(*USFun)(JsonObject);
typedef void(*LoopFun)(JsonObject, uint8_t*);

struct UserLoop {
  JsonObject object;
  LoopFun loopFun;
  size_t bufSize = 100;
  uint16_t interval = 160; //160ms default
  unsigned long lastMillis = 0;
  unsigned long counter = 0;
  unsigned long prevCounter = 0;
};

class SysModUI:public Module {

public:
  static std::vector<USFun> uiFunctions;
  static std::vector<UserLoop> loopFunctions;

  static bool userLoopsChanged;

  SysModUI();

  //serve index.htm
  void setup();

  void loop();

  JsonObject initGroup(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "group", uiFun, chFun, loopFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initMany(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "many", uiFun, chFun, loopFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initInput(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "input", uiFun, chFun, loopFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "password", uiFun, chFun, loopFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "number", uiFun, chFun, loopFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initSlider(JsonObject parent, const char * id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "range", uiFun, chFun, loopFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "canvas", uiFun, chFun, loopFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initDisplay(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "display", uiFun, chFun, loopFun);
    if (object["value"].isNull() && value) object["value"] = value;
    if (chFun && value) chFun(object);
    return object;
  }

  JsonObject initCheckBox(JsonObject parent, const char * id, bool value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "checkbox", uiFun, chFun, loopFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initButton(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "button", uiFun, chFun, loopFun);
    if (object["value"].isNull()) object["value"] = value;
    //no call of fun for buttons!!! 
    // if (chFun) chFun(object);
    return object;
  }

  JsonObject initDropdown(JsonObject parent, const char * id, uint8_t value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, "dropdown", uiFun, chFun, loopFun);
    if (object["value"].isNull()) object["value"] = value;
    if (chFun) chFun(object);
    return object;
  }

  JsonObject initObject(JsonObject parent, const char * id, const char * type, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  //setValue char
  static JsonObject setValue(const char * id, const char * value);

  //setValue int
  static JsonObject setValue(const char * id, int value);

  //setValue bool
  static JsonObject setValue(const char * id, bool value);

  //run the change function and send response to all? websocket clients
  static void setChFunAndWs(JsonObject object, const char * value = nullptr);

  //Set value with argument list
  static JsonObject setValueV(const char * id, const char * format, ...) { //static to use in *Fun
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);

    va_end(args);

    return setValue(id, value);
  }

  //Set value with argument list and print
  JsonObject setValueP(const char * id, const char * format, ...);

  JsonVariant getValue(const char * id);

  static JsonObject findObject(const char * id, JsonArray parent = JsonArray());

  //interpret json and run commands or set values
  static const char * processJson(JsonVariant &json);
};

static SysModUI *ui;