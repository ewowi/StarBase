#include <vector>

#include "Module.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModUI.h"

#include "FastLED.h"

#define DATA_PIN 16
#define NUM_LEDS 256

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

static CRGB leds[NUM_LEDS];
static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static uint16_t nrOfLeds = 64; 
static uint16_t fps = 40;

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
  unsigned long frameCounter = 0;

  AppModLeds() :Module("Leds") {};

  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);

    ui->initNumber(parentObject, "dataPin", dataPin, [](JsonObject object) {
      web->addResponse(object, "comment", "Not implemented yet (fixed to 16)");
    }, [](JsonObject object) {
      print->print("Set data pin to %d\n", object["value"].as<int>());
    });

    ui->initNumber(parentObject, "nrOfLeds", nrOfLeds, [](JsonObject object) {
      web->addResponse(object, "comment", "Currenntly max 256");
    }, [](JsonObject object) {
      fadeToBlackBy( leds, nrOfLeds, 100);
      nrOfLeds = object["value"];
      print->print("Set nrOfLeds to %d\n", nrOfLeds);
    });

    ui->initNumber(parentObject, "bri", 5, [](JsonObject object) {
      web->addResponse(object, "label", "Brightness");
    }, [](JsonObject object) {
      FastLED.setBrightness(object["value"]);
      print->print("Set Brightness to %d\n", object["value"].as<int>());
    });

    ui->initNumber(parentObject, "fps", fps, [](JsonObject object) {
      web->addResponse(object, "comment", "Frames per second");
    }, [](JsonObject object) {
      fps = object["value"];
      print->print("fps changed %d\n", fps);
    });

    ui->initDropdown(parentObject, "fx", 3, [](JsonObject object) {
      web->addResponse(object, "label", "Effect");
      web->addResponse(object, "comment", "Effect to show");
      JsonArray lov = web->addResponseArray(object, "lov");
      for (Effect *effect:effects) {
        lov.add(effect->name());
      }
    }, [](JsonObject object) {
      print->print("%s Running %s\n", __PRETTY_FUNCTION__, object["id"].as<const char *>());
    });

    ui->initDisplay(parentObject, "realFps");

    // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 

    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new JuggleEffect);

    for (Effect *effect:effects) {
      print->print("Size of %s is %d\n", effect->name(), sizeof(*effect));
    }

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    if(millis() - nowUp > 1000/fps) {
      nowUp = millis();

      Effect* effect = effects[ui->getValue("fx")];
      effect->loop();

      // yield();
      FastLED.show();  

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