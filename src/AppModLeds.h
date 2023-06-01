#include "Module.h"

#include "FastLED.h"

#define DATA_PIN 1
#define NUM_LEDS 1

class AppModLeds:public Module {

public:
  CRGB leds[1];
  uint8_t dataPin = 16; 
  uint8_t ledCount = 30; 

  AppModLeds() :Module("Leds") {}; //constructor

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    ui->defGroup(name);
    ui->defNumber("dataPin", dataPin, [](const char *prompt, JsonVariant value) {
      print->print("Set data pin\n");
    });
    ui->defNumber("ledCount", ledCount, [](const char *prompt, JsonVariant value) {
      print->print("Set ledCount\n");
    });
    ui->defButton("Effect 1", "Effect 1", [](const char *prompt, JsonVariant value) {
      print->print("Running effect 1\n");
    });
    ui->defButton("Effect 2", "Effect 2", [](const char *prompt, JsonVariant value) {
      print->print("Running effect 2\n");
    });

      // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    // leds[0] = CRGB::White; FastLED.show(); delay(30); 
    // leds[0] = CRGB::Black; FastLED.show(); delay(30);
  }

};

static AppModLeds *lds;