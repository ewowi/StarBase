#include "Module.h"

#include "FastLED.h"

#define DATA_PIN 16
#define NUM_LEDS 256

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

static CRGB leds[NUM_LEDS];
static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static uint16_t nrOfLeds = 64; 

class Effect {
public:
  virtual const char * name() { return nullptr;}
  virtual void setup() {} //not implemented yet
  virtual void loop() {}
  virtual const char *parameters() {return nullptr;}
};

class RainbowEffect:public Effect {
public:
  const char *name() {
    return "Rainbow";
  }
  void setup() {} //not implemented yet
  void loop() {
    // FastLED's built-in rainbow generator
    fill_rainbow( leds, nrOfLeds, gHue, 7);
  }
};

class RainbowWithGlitterEffect:public RainbowEffect {
public:
  const char *name() {
    return "Rainbow with glitter";
  }
  void setup() {} //not implemented yet
  void loop() {
    // built-in FastLED rainbow, plus some random sparkly glitter
    RainbowEffect::loop();
    addGlitter(80);
  }
  static void addGlitter( fract8 chanceOfGlitter) 
  {
    if( random8() < chanceOfGlitter) {
      leds[ random16(nrOfLeds) ] += CRGB::White;
    }
  }
};

class SinelonEffect:public Effect {
public:
  const char *name() {
    return "Sinelon";
  }
  void setup() {} //not implemented yet
  void loop() {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( leds, nrOfLeds, 20);
    int pos = beatsin16( 13, 0, nrOfLeds-1 );
    leds[pos] += CHSV( gHue, 255, 192);
  }
};

class ConfettiEffect:public Effect {
public:
  const char *name() {
    return "Confetti";
  }
  void setup() {} //not implemented yet
  void loop() {
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy( leds, nrOfLeds, 10);
    int pos = random16(nrOfLeds);
    leds[pos] += CHSV( gHue + random8(64), 200, 255);
  }
};

class BPMEffect:public Effect {
public:
  const char *name() {
    return "Beats per minute";
  }
  void setup() {} //not implemented yet
  void loop() {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < nrOfLeds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
    }
  }
  const char *parameters() {
    return "BeatsPerMinute";
  }
};

class JuggleEffect:public Effect {
public:
  const char *name() {
    return "Juggle";
  }
  void setup() {} //not implemented yet
  void loop() {
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy( leds, nrOfLeds, 20);
    uint8_t dothue = 0;
    for( int i = 0; i < 8; i++) {
      leds[beatsin16( i+7, 0, nrOfLeds-1 )] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
};

static std::vector<Effect *> effects;

class AppModLeds:public Module {

public:
  uint8_t dataPin = 16; 
  unsigned long nowUp = 0;
  unsigned long secondMillis = 0;
  unsigned long frameCounter = 0;

  // List of patterns to cycle through.  Each is defined as a separate function below.

  static uint8_t effectNr; // Index number of which pattern is current
  
  static uint8_t brightness;
  static uint16_t fps;
  static unsigned long realFps;

  AppModLeds() :Module("Leds") {};

  void setup() {
    Module::setup();
    print->print("%s Setup:", name);

    parentObject = ui->initGroup(JsonObject(), name);
    ui->initNumber(parentObject, "dataPin", dataPin, [](const char *prompt, JsonVariant value) {
      print->print("Set data pin to %d\n", value.as<int>());
    });
    ui->initNumber(parentObject, "nrOfLeds", nrOfLeds, [](const char *prompt, JsonVariant value) {
      fadeToBlackBy( leds, nrOfLeds, 100);
      nrOfLeds = value;
      print->print("Set nrOfLeds to %d\n", nrOfLeds);
    });
    ui->initNumber(parentObject, "Brightness", brightness, [](const char *prompt, JsonVariant value) {
      brightness = value;
      FastLED.setBrightness(brightness);
      print->print("Set Brightness to %d\n", brightness);
    });
    ui->initNumber(parentObject, "FPS", fps, [](const char *prompt, JsonVariant value) {
      fps = value;
      print->print("fps changed %d\n", fps);
    });

    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new JuggleEffect);

    for (Effect *effect:effects) {
      print->print("Size of %s is %d\n", effect->name(), sizeof(*effect));
    }
    ui->initDropdown(parentObject, "Effect", effectNr, [](const char *prompt, JsonVariant value) {
      effectNr = value;
      print->print("Running %s\n", prompt);
    }, [](const char *prompt) {
      responseDoc["label"] = prompt;
      responseDoc["comment"] = "comment";
      JsonArray lov = responseDoc.createNestedArray("lov");
      for (Effect *effect:effects) {
        lov.add(effect->name());
      }
      return responseDoc.as<JsonVariant>();
    });

    ui->initDisplay(parentObject, "realFps");

    // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 

    FastLED.setBrightness(brightness);

    print->print("%s %s\n", name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    if(millis() - nowUp > 1000/fps) {
      nowUp = millis();

      Effect* effect = effects[effectNr];
      effect->loop();

      yield();
      FastLED.show();  
      // insert a delay to keep the framerate modest
      // FastLED.delay(1000/fps); 
      frameCounter++;
    }
    if (millis() - secondMillis >= 1000 || !secondMillis) {
      secondMillis = millis();
      ui->setValueV("realFps", "%lu /s", frameCounter);
      frameCounter = 0;
    }

    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  }

};

static AppModLeds *lds;

// CRGB AppModLeds::leds[NUM_LEDS];
uint8_t AppModLeds::brightness = 5;
uint16_t AppModLeds::fps = 40;
// uint16_t AppModLeds::nrOfLeds = 64;

uint8_t AppModLeds::effectNr = 3; // Index number of which pattern is current
// uint8_t AppModLeds::gHue = 0;

unsigned long AppModLeds::realFps = 0;
