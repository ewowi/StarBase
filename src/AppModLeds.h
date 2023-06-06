#include "Module.h"

#include "FastLED.h"

#define DATA_PIN 16
#define NUM_LEDS 256

// https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

class AppModLeds:public Module {

public:
  static CRGB leds[NUM_LEDS];
  uint8_t dataPin = 16; 
  unsigned long nowUp = 0;

  // List of patterns to cycle through.  Each is defined as a separate function below.
  std::vector<void(*)()> gPatterns;

  static uint8_t gCurrentPatternNumber; // Index number of which pattern is current
  static uint8_t gHue; // rotating "base color" used by many of the patterns
  
  static uint8_t brightness;
  static uint16_t fps;
  static uint16_t nrOfLeds; 

  AppModLeds() :Module("Leds") {};

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    parentObject = ui->initGroup(JsonObject(), name);
    ui->initNumber(parentObject, "dataPin", dataPin, [](const char *prompt, JsonVariant value) {
      print->print("Set data pin\n");
    });
    ui->initNumber(parentObject, "nrOfLeds", nrOfLeds, [](const char *prompt, JsonVariant value) {
      fadeToBlackBy( leds, nrOfLeds, 100);
      nrOfLeds = value;
      print->print("Set nrOfLeds to %d\n", nrOfLeds);
    });
    ui->initNumber(parentObject, "Brightness", brightness, [](const char *prompt, JsonVariant value) {
      brightness = value;
      FastLED.setBrightness(brightness);
    });
    ui->initNumber(parentObject, "FPS", fps, [](const char *prompt, JsonVariant value) {
      fps = value;
      print->print("fps changed %d\n", fps);
    });
    ui->initButton(parentObject, "Rainbow", [](const char *prompt, JsonVariant value) {
      gCurrentPatternNumber = 0;
      print->print("Running effect 1\n");
    });
    ui->initButton(parentObject, "rainbowWithGlitter", [](const char *prompt, JsonVariant value) {
      gCurrentPatternNumber = 1;
      print->print("Running effect 1\n");
    });
    ui->initButton(parentObject, "confetti", [](const char *prompt, JsonVariant value) {
      gCurrentPatternNumber = 2;
      print->print("Running effect 1\n");
    });
    ui->initButton(parentObject, "sinelon", [](const char *prompt, JsonVariant value) {
      gCurrentPatternNumber = 3;
      print->print("Running effect 1\n");
    });
    ui->initButton(parentObject, "juggle", [](const char *prompt, JsonVariant value) {
      gCurrentPatternNumber = 4;
      print->print("Running effect 1\n");
    });
    ui->initButton(parentObject, "bpm", [](const char *prompt, JsonVariant value) {
      gCurrentPatternNumber = 5;
      print->print("Running effect 1\n");
    });

    gPatterns.push_back(rainbow);
    gPatterns.push_back(rainbowWithGlitter);
    gPatterns.push_back(confetti); 
    gPatterns.push_back(sinelon); 
    gPatterns.push_back(juggle); 
    gPatterns.push_back(bpm);

      // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 

    FastLED.setBrightness(brightness);

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    if(millis() - nowUp > 1000/fps) {
      nowUp = millis();

      gPatterns[gCurrentPatternNumber]();

      yield();
      FastLED.show();  
      // insert a delay to keep the framerate modest
      // FastLED.delay(1000/fps); 

    }
    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  }

  static void rainbow() 
  {
    // FastLED's built-in rainbow generator
    fill_rainbow( leds, NUM_LEDS, gHue, 7);
  }

  static void rainbowWithGlitter() 
  {
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter(80);
  }

  static void addGlitter( fract8 chanceOfGlitter) 
  {
    if( random8() < chanceOfGlitter) {
      leds[ random16(NUM_LEDS) ] += CRGB::White;
    }
  }

  static void confetti() 
  {
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy( leds, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV( gHue + random8(64), 200, 255);
  }

  static void sinelon()
  {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( leds, NUM_LEDS, 20);
    int pos = beatsin16( 13, 0, NUM_LEDS-1 );
    leds[pos] += CHSV( gHue, 255, 192);
  }

  static void bpm()
  {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < NUM_LEDS; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
    }
  }

  static void juggle() {
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy( leds, NUM_LEDS, 20);
    uint8_t dothue = 0;
    for( int i = 0; i < 8; i++) {
      leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
};

static AppModLeds *lds;

CRGB AppModLeds::leds[NUM_LEDS];
uint8_t AppModLeds::brightness = 16;
uint16_t AppModLeds::fps = 40;
uint16_t AppModLeds::nrOfLeds = 64;

uint8_t AppModLeds::gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t AppModLeds::gHue = 0;
  