#pragma once
#include <vector>
#include "ArduinoJson.h"
#include "Module.h"


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

  static int objectCounter; //not static crashes ??? (not called async...?)

  SysModUI();

  //serve index.htm
  void setup();

  void loop();

  JsonObject initGroup(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initMany(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initInput(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initNumber(JsonObject parent, const char * id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initSlider(JsonObject parent, const char * id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initCanvas(JsonObject parent, const char * id, int value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initDisplay(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initCheckBox(JsonObject parent, const char * id, bool value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initButton(JsonObject parent, const char * id, const char * value = nullptr, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initDropdown(JsonObject parent, const char * id, uint8_t value, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  JsonObject initObject(JsonObject parent, const char * id, const char * type, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr);

  //run the change function and send response to all? websocket clients
  static void setChFunAndWs(JsonObject object, const char * value = nullptr);

  //interpret json and run commands or set values
  static const char * processJson(JsonVariant &json);

  void processUiFun(const char * id);

};

static SysModUI *ui;