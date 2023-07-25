#pragma once
#include <vector>
#include "ArduinoJson.h"
#include "Module.h"

typedef void(*UCFun)(JsonObject);
typedef void(*LoopFun)(JsonObject, uint8_t*);

struct ObjectLoop {
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
  SysModUI();

  //serve index.htm
  void setup();

  void loop();

  JsonObject initModule(JsonObject parent, const char * id, const char * value = nullptr, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<const char *>(parent, id, "module", value, true, uiFun, chFun, loopFun);
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<const char *>(parent, id, "table", value, false, uiFun, chFun, loopFun);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<const char *>(parent, id, "text", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<const char *>(parent, id, "password", value, false, uiFun, chFun, loopFun);
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<int>(parent, id, "number", value, false, uiFun, chFun, loopFun);
  }

  JsonObject initSlider(JsonObject parent, const char * id, int value, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<int>(parent, id, "range", value, false, uiFun, chFun, loopFun);
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<int>(parent, id, "canvas", value, false, uiFun, chFun, loopFun);
  }

  JsonObject initCheckBox(JsonObject parent, const char * id, bool value, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<bool>(parent, id, "checkbox", value, false, uiFun, chFun, loopFun);
  }

  JsonObject initButton(JsonObject parent, const char * id, const char * value = nullptr, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<const char *>(parent, id, "button", value, true, uiFun, chFun, loopFun);
  }

  JsonObject initSelect(JsonObject parent, const char * id, uint8_t value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<uint8_t>(parent, id, "select", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initObjectAndUpdate<const char *>(parent, id, "textarea", value, readOnly, uiFun, chFun, loopFun);
  }

  template <typename Type>
  JsonObject initObjectAndUpdate(JsonObject parent, const char * id, const char * type, Type value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = initObject(parent, id, type, readOnly, uiFun, chFun, loopFun);
    bool isPointer = std::is_pointer<Type>::value;
    if (object["value"].isNull() && (!isPointer || value)) object["value"] = value; //if value is a pointer, it needs to have a value
    //tbd check if value in case of constchar* needs to be copied using (char *)...
    //no call of fun for buttons otherwise all buttons will be fired including restart delete model.json and all that jazz!!! 
    if (strcmp(type,"button")!=0 && chFun && value) chFun(object);
    return object;
  }

  JsonObject initObject(JsonObject parent, const char * id, const char * type, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr);

  //run the change function and send response to all? websocket clients
  static void setChFunAndWs(JsonObject object, const char * value = nullptr);

  //interpret json and run commands or set values
  static const char * processJson(JsonVariant &json);

  void processUiFun(const char * id);

private:
  static bool objectLoopsChanged;

  static int objectCounter; //not static crashes ??? (not called async...?)

  static std::vector<UCFun> ucFunctions;
  static std::vector<ObjectLoop> loopFunctions;

};

static SysModUI *ui;