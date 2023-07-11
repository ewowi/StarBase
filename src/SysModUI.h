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

  // static bool userLoopsChanged;

  SysModUI() :Module("UI") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    success &= web->addURL("/", "/index.htm", "text/html");
    // success &= web->addURL("/index.js", "/index.js", "text/javascript");
    // success &= web->addURL("/index.css", "/index.css", "text/css");

    success &= web->setupJsonHandlers("/json", processJson);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //serve index.htm
  void setup() {
    Module::setup();

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = initGroup(parentObject, name);
    initDisplay(parentObject, "uloops", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "User loops");
    });

    // JsonObject userLoopsObject = initMany(parentObject, "uloops", nullptr, [](JsonObject object) { //uiFun
    //   web->addResponse(object, "label", "User loops");
    //   JsonArray rows = web->addResponseArray(object, "many");

    //   for (auto userLoop = begin (loopFunctions); userLoop != end (loopFunctions); ++userLoop) {
    //     JsonArray row = rows.createNestedArray();
    //     row.add(userLoop->object["id"]);
    //     row.add(userLoop->counter);
    //     userLoopsChanged = userLoop->counter != userLoop->prevCounter;
    //     userLoop->prevCounter = userLoop->counter;
    //     userLoop->counter = 0;
    //   }
    // });
    // initDisplay(userLoopsObject, "ulObject", nullptr, [](JsonObject object) { //uiFun
    //   web->addResponse(object, "label", "Name");
    // });
    // initDisplay(userLoopsObject, "ulLoopps", nullptr, [](JsonObject object) { //uiFun
    //   web->addResponse(object, "label", "Loops/s");
    // });

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    for (auto userLoop = begin (loopFunctions); userLoop != end (loopFunctions); ++userLoop) {
      if (millis() - userLoop->lastMillis >= userLoop->interval) {
        userLoop->lastMillis = millis();

        ws.cleanupClients();
        if (ws.count()) {

          //send object to notify client data coming is for object (client then knows it is canvas and expects data for it)
          // print->printObject(object); print->print("\n");
          responseDoc.clear(); //needed for deserializeJson?
          JsonString id = userLoop->object["id"];
          if (responseDoc[id].isNull()) responseDoc.createNestedObject(id);
          responseDoc[id]["value"] = true;
          // web->addResponse(object, "value", object["value"]);
          web->sendDataWs(responseDoc.as<JsonVariant>());

          // print->print("bufSize %d", userLoop->bufSize);

          //send leds info in binary data format

          AsyncWebSocketMessageBuffer * wsBuf = ws.makeBuffer(userLoop->bufSize * 3 + 4);

          if (wsBuf) {//out of memory
            wsBuf->lock();
            uint8_t* buffer = wsBuf->get();

            //to loop over old size
            buffer[0] = userLoop->bufSize / 256;
            buffer[1] = userLoop->bufSize % 256;
            // print->print("interval1 %u %d %d %d %d %d %d\n", millis(), userLoop->interval, userLoop->bufSize, buffer[0], buffer[1]);

            userLoop->loopFun(userLoop->object, buffer); //call the function and fill the buffer

            userLoop->bufSize = buffer[0] * buffer[1] * buffer[2];
            userLoop->interval = buffer[3]*10; //from cs to ms
            // print->print("interval2 %u %d %d %d %d %d %d\n", millis(), userLoop->interval, userLoop->bufSize, buffer[0], buffer[1], buffer[2], buffer[3]);

            try {
              for (auto client:ws.getClients()) {
                if (!client->queueIsFull() && client->status() == WS_CONNECTED) 
                  client->binary(wsBuf);
                else {
                  // web->clientsChanged = true; tbd: changed also if full status changes
                  print->print("loopFun client %s full or not connected\n", client->remoteIP().toString().c_str());
                }
              }
              // throw (1); // Throw an exception when a problem arise
            }
            catch (...) {
              Serial.printf("BINARY ALL EXCEPTION\n");
            }
            wsBuf->unlock();
            ws._cleanBuffers();
          }
          else {
            print->print("loopFun WS buffer allocation failed\n");
            ws.closeAll(1013); //code 1013 = temporary overload, try again later
            ws.cleanupClients(0); //disconnect all clients to release memory
            ws._cleanBuffers();
          }
        }

        userLoop->counter++;
        // print->print("%s %u %u %d %d\n", userLoop->object["id"].as<const char *>(), userLoop->lastMillis, millis(), userLoop->interval, userLoop->counter);
      }
    }

    if (millis() - secondMillis >= 1000 || !secondMillis) {
      secondMillis = millis();

      setValueV("uloops", "%lu /s", loopFunctions[loopFunctions.size()-1].counter);
      loopFunctions[loopFunctions.size()-1].counter = 0;

      // //replace uloops table
      // responseDoc.clear(); //needed for deserializeJson?
      // responseDoc["uiFun"] = "uloops";
      // JsonVariant responseVariant = responseDoc.as<JsonVariant>();
      // processJson(responseVariant); //this calls uiFun command, which might change userLoopsChanged

      // //if something changed in uloops
      // if (userLoopsChanged) {
      //   userLoopsChanged = false;
      //   print->printJson("uloops change response", responseDoc);
      //   web->sendDataWs(responseVariant);
      // }
    }
  }

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

  JsonObject initObject(JsonObject parent, const char * id, const char * type, USFun uiFun = nullptr, USFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject object = findObject(id);

    //create new object
    if (object.isNull()) {
      print->print("initObject create new %s: %s\n", type, id);
      if (parent.isNull()) {
        JsonArray objects = model.as<JsonArray>();
        object = objects.createNestedObject();
      } else {
        if (parent["n"].isNull()) parent.createNestedArray("n"); //if parent exist and no "n" array, create it
        object = parent["n"].createNestedObject();
        // serializeJson(model, Serial);Serial.println();
      }
      object["id"] = id;
    }
    else
      print->print("Object %s already defined\n", id);

    if (!object.isNull()) {
      object["s"] = true; //deal with obsolete objects, check object exists in code
      object["type"] = type;
      if (uiFun) {
        //if fun already in uiFunctions then reuse, otherwise add new fun in uiFunctions
        std::vector<void(*)(JsonObject object)>::iterator itr = find(uiFunctions.begin(), uiFunctions.end(), uiFun);
        if (itr!=uiFunctions.end()) //found
          object["uiFun"] = distance(uiFunctions.begin(), itr); //assign found function
        else { //not found
          uiFunctions.push_back(uiFun); //add new function
          object["uiFun"] = uiFunctions.size()-1;
        }
      }
      if (chFun) {
        //if fun already in uiFunctions then reuse, otherwise add new fun in uiFunctions
        std::vector<void(*)(JsonObject object)>::iterator itr = find(uiFunctions.begin(), uiFunctions.end(), chFun);
        if (itr!=uiFunctions.end()) //found
          object["chFun"] = distance(uiFunctions.begin(), itr); //assign found function
        else { //not found
          uiFunctions.push_back(chFun); //add new function
          object["chFun"] = uiFunctions.size()-1;
        }
      }
      if (loopFun) {
        UserLoop loop;
        loop.loopFun = loopFun;
        loop.object = object;

        loopFunctions.push_back(loop);
        object["loopFun"] = loopFunctions.size()-1;
        // userLoopsChanged = true;
        // print->print("iObject loopFun %s %u %u %d %d\n", object["id"].as<const char *>());
      }
    }
    else
      print->print("initObject could not find or create object %s with %s\n", id, type);

    return object;
  }

  //setValue char
  static JsonObject setValue(const char * id, const char * value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
        if (object["type"] == "display") { // do not update object["value"]
          if (!object["chFun"].isNull()) {//isnull needed here!
            size_t funNr = object["chFun"];
            uiFunctions[funNr](object);
          }

          responseDoc.clear(); //needed for deserializeJson?
          web->addResponse(object, "value", value);
          web->sendDataWs(responseDoc.as<JsonVariant>());
        } else {
          object["value"] = (char *)value; //(char *) forces a copy (https://arduinojson.org/v6/api/jsonvariant/subscript/) (otherwise crash!!)
          setChFunAndWs(object);
        }
      }
    }
    else
      print->print("setValue Object %s not found\n", id);
    return object;
  }

  //setValue int
  static JsonObject setValue(const char * id, int value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value);
        object["value"] = value;
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", id);

    return object;
  }

  //setValue bool
  static JsonObject setValue(const char * id, bool value) {
    JsonObject object = findObject(id);
    if (!object.isNull()) {
      if (object["value"].isNull() || object["value"] != value) {
        // print->print("setValue changed %s %s->%s\n", id, object["value"].as<String>().c_str(), value?"true":"false");
        object["value"] = value;
        setChFunAndWs(object);
      }
    }
    else
      print->print("setValue Object %s not found\n", id);
    return object;
  }

  //run the change function and send response to all? websocket clients
  static void setChFunAndWs(JsonObject object) {

    if (!object["chFun"].isNull()) {//isnull needed here!
      size_t funNr = object["chFun"];
      uiFunctions[funNr](object);
    }

    responseDoc.clear(); //needed for deserializeJson?
    if (object["value"].is<int>())
      web->addResponseInt(object, "value", object["value"].as<int>());
    else if (object["value"].is<bool>())
      web->addResponseBool(object, "value", object["value"].as<bool>());
    else if (object["value"].is<const char *>())
      web->addResponse(object, "value", object["value"].as<const char *>());
    else {
      print->print("unknown type for %s\n", object["value"].as<String>().c_str());
      web->addResponse(object, "value", object["value"]);
    }
    // if (object["id"] == "pview" || object["id"] == "fx") {
    //   print->printJson("setChFunAndWs response", responseDoc);
    // }

    web->sendDataWs(responseDoc.as<JsonVariant>());
  }

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
  JsonObject setValueP(const char * id, const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[100];
    vsnprintf(value, sizeof(value), format, args);
    print->print("%s\n", value);

    va_end(args);

    return setValue(id, value);
  }

  JsonVariant getValue(const char * id) {
    JsonObject object = findObject(id);
    if (!object.isNull())
      return object["value"];
    else {
      print->print("Value of %s does not exist!!\n", id);
      return JsonVariant();
    }
  }

  static JsonObject findObject(const char * id, JsonArray parent = JsonArray()) { //static for processJson
    JsonArray root;
    // print ->print("findObject %s %s\n", id, parent.isNull()?"root":"n");
    if (parent.isNull()) {
      root = model.as<JsonArray>();
    }
    else {
      root = parent;
    }
    JsonObject foundObject;
    for(JsonObject object : root) {
      if (foundObject.isNull()) {
        if (object["id"] == id)
          foundObject = object;
        else if (!object["n"].isNull())
          foundObject = findObject(id, object["n"]);
      }
    }
    return foundObject;
  }

  static const char * processJson(JsonVariant &json) { //static for setupJsonHandlers
    if (json.is<JsonObject>()) //should be
    {
      for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
        const char * key = pair.key().c_str();
        JsonVariant value = pair.value();

        // commands
        if (pair.key() == "uiFun") { //JsonString can do ==
          //find the dropdown object and collect it's options...
          JsonObject object = findObject(value); //value is the id
          if (!object.isNull()) {
            //call ui function...
            if (!object["uiFun"].isNull()) {//isnull needed here!
              size_t funNr = object["uiFun"];
              uiFunctions[funNr](object);
              if (object["type"] == "dropdown")
                web->addResponseInt(object, "value", object["value"]); //temp assume int only

              // print->printJson("PJ Command", responseDoc);
            }
          }
          else
            print->print("PJ Command %s object %s not found\n", key, value.as<String>().c_str());
        } 
        else { //normal change
          if (!value.is<JsonObject>()) { //no objects (inserted by uiFun responses)
            JsonObject object = findObject(key);
            if (!object.isNull())
            {
              if (object["value"] != value) { // if changed
                // print->print("processJson %s %s->%s\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());

                //set new value
                if (value.is<const char *>())
                  setValue(key, value.as<const char *>());
                else if (value.is<bool>())
                  setValue(key, value.as<bool>());
                else if (value.is<int>())
                  setValue(key, value.as<int>());
                else {
                  print->print("processJson %s %s->%s not a supported type yet\n", key, object["value"].as<String>().c_str(), value.as<String>().c_str());
                }
              }
              else if (object["type"] == "button")
                setChFunAndWs(object);
            }
            else
              print->print("Object %s not found\n", key);
          }
        }
      } //for json pairs
    }
    else
      print->print("Json not object???\n");
    return nullptr;
  }

};

static SysModUI *ui;

//init static variables (https://www.tutorialspoint.com/cplusplus/cpp_static_members.htm)
std::vector<void(*)(JsonObject object)> SysModUI::uiFunctions;
std::vector<UserLoop> SysModUI::loopFunctions;

// bool SysModUI::userLoopsChanged = false;;