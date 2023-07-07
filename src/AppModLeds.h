#include "FastLED.h"

#define DATA_PIN 16
#define NUM_LEDS 256

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

static CRGB leds[NUM_LEDS];
static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static uint16_t nrOfLeds = 64; 
static uint16_t fps = 40;

//should not contain bytes to keep mem as small as possible
class Effect {
public:
  unsigned long call = 0;
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

class RunningEffect:public Effect {
public:
  const char *name() {
    return "Running";
  }
  void setup() {} //not implemented yet
  void loop() {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( leds, nrOfLeds, 70);
    // int pos0 = (call-1)%nrOfLeds;
    // leds[pos0] = CHSV( 0,0,0);
    int pos = call%nrOfLeds;
    leds[pos] = CHSV( gHue, 255, 192);
    call++;
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

    ui->initNumber(parentObject, "dataPin", dataPin, [](JsonObject object) { //uiFun
      web->addResponseV(object, "comment", "Not implemented yet (fixed to %d)", DATA_PIN);
    }, [](JsonObject object) { //chFun
      print->print("Set data pin to %d\n", object["value"].as<int>());
    });

    ui->initNumber(parentObject, "nrOfLeds", nrOfLeds, [](JsonObject object) { //uiFun
      web->addResponseV(object, "comment", "Max %d", NUM_LEDS);
    }, [](JsonObject object) { //chFun

      fadeToBlackBy( leds, nrOfLeds, 100);
      nrOfLeds = object["value"];
      if (nrOfLeds>NUM_LEDS) {nrOfLeds = NUM_LEDS;ui->setValue("nrOfLeds", NUM_LEDS);};

      //change nrOfLeds in pview
      JsonObject pvObject = ui->findObject("pview");
      if (!pvObject.isNull() && !pvObject["loopFun"].isNull()) {
        size_t index = pvObject["loopFun"];
        if (index >= 0 && index < ui->loopFunctions.size()) {
          ui->loopFunctions[index].bufSize = nrOfLeds;
          ui->loopFunctions[index].interval = max((int)(nrOfLeds * ws.count() / 20), 80);
        }
        else {
          print->print("pview not right loopFun Index %d\n", index);
        }
      }
      else {
        print->print("pview not found\n");
      }

      print->print("Set nrOfLeds to %d\n", nrOfLeds);

    });

    ui->initSlider(parentObject, "bri", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Brightness");
    }, [](JsonObject object) { //chFun
      uint8_t bri = map(object["value"], 0, 100, 0, 255);
      FastLED.setBrightness(bri);
      print->print("Set Brightness to %d -> %d\n", object["value"].as<int>(), bri);
    });

    ui->initCanvas(parentObject, "pview", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Preview");
      web->addResponse(object, "comment", "Preview in 2D");
    }, nullptr, [](JsonObject object, uint8_t* buffer) { //loopFun
      // send leds preview to clients
        buffer[0] = sqrt(nrOfLeds);
        buffer[1] = nrOfLeds / buffer[0];
        buffer[2] = 1;
        for (size_t i = 0; i < nrOfLeds; i++)
        {
          buffer[i*3+3] = leds[i].red;
          buffer[i*3+3+1] = leds[i].green;
          buffer[i*3+3+2] = leds[i].blue;
        }
    }, nrOfLeds, max((int)(nrOfLeds * ws.count() / 20), 80)); //bufSize and loop interval: not too fast (changed when nrofLeds change) publish subscribe mechanism?

    ui->initNumber(parentObject, "fps", fps, [](JsonObject object) { //uiFun
      web->addResponse(object, "comment", "Frames per second");
    }, [](JsonObject object) { //chFun
      fps = object["value"];
      print->print("fps changed %d\n", fps);
    });

    ui->initDropdown(parentObject, "fx", 3, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Effect");
      web->addResponse(object, "comment", "Effect to show");
      JsonArray lov = web->addResponseArray(object, "lov");
      for (Effect *effect:effects) {
        lov.add(effect->name());
      }
    }, [](JsonObject object) { //chFun
      print->print("%s Change %s to %d\n", "initDropdown chFun", object["id"].as<const char *>(), object["value"].as<int>());
    });

    ui->initDisplay(parentObject, "realFps");

    // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 

    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new RunningEffect);
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