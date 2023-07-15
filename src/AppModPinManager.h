class AppModPinManager:public Module {

public:

  AppModPinManager() :Module("Pin Manager") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    pinMode(2, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(33, OUTPUT);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);

    ui->initCanvas(parentObject, "board", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Board layout");
    }, nullptr, [](JsonObject object, uint8_t* buffer) { //loopFun
      // send leds preview to clients
      for (size_t i = 0; i < buffer[0] * 256 + buffer[1]; i++)
      {
        buffer[i*3+4] = (digitalRead(i)+1) * 50;
        buffer[i*3+4+1] = 255;
        buffer[i*3+4+2] = 192;
      }
      //new values
      buffer[0] = 10;
      buffer[1] = 2;
      buffer[2] = 1;
      buffer[3] = 10*10; //every 10 sec 
    });

    ui->initCheckBox(parentObject, "pin2", true, nullptr, updateGPIO);
    ui->initCheckBox(parentObject, "pin4", false);
    ui->initCheckBox(parentObject, "pin33", true);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

  static void updateGPIO(JsonObject object) {
    if (object["value"].is<bool>()) {
      bool pin = object["value"];
      JsonString id = object["id"];

      print->print("updateGPIO %s:=%d\n", id.c_str(), pin);

      if (id == "pin2") digitalWrite(2, pin?HIGH:LOW);
      if (id == "pin4") digitalWrite(4, pin?HIGH:LOW);
      if (id == "pin33") digitalWrite(33, pin?HIGH:LOW);
    }
  }

};

static AppModPinManager *pin;