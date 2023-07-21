#include "FastLED.h"

#define DATA_PIN 16
#define NUM_LEDS 1024

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

// CRGB *leds = nullptr;
CRGB leds[NUM_LEDS];
static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static uint16_t nrOfLeds = 0; 
static uint16_t width = 8; 
static uint8_t height = 8; 
static uint8_t depth = 1; 
static uint16_t fps = 40;
static unsigned long call = 0;
uint8_t bri = 10;

//should not contain bytes to keep mem as small as possible
class Effect {
public:
  virtual const char * name() { return nullptr;}
  virtual void setup() {} //not implemented yet
  virtual void loop() {}
  virtual const char * parameters() {return nullptr;}
};

class RainbowEffect:public Effect {
public:
  const char * name() {
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
  const char * name() {
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
  const char * name() {
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
  const char * name() {
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
  }
};

class ConfettiEffect:public Effect {
public:
  const char * name() {
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
  const char * name() {
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
  const char * parameters() {
    return "BeatsPerMinute";
  }
};

class JuggleEffect:public Effect {
public:
  const char * name() {
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

float distance(uint16_t x1, uint16_t y1, uint16_t z1, uint16_t x2, uint16_t y2, uint16_t z2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
}

class Ripples3DEffect:public Effect {
public:
  const char * name() {
    return "Ripples 3D";
  }
  void setup() {} //not implemented yet
  void loop() {
    float ripple_interval = 1.3;// * (SEGMENT.intensity/128.0);

    fill_solid(leds, nrOfLeds, CRGB::Black);
    // fill(CRGB::Black);

    uint16_t mW = width;
    uint16_t mH = height;
    uint16_t mD = depth;

    for (int z=0; z<mD; z++) {
        for (int x=0; x<mW; x++) {
            float d = distance(3.5, 3.5, 0, x, z, 0)/9.899495*mH;
            uint16_t height = floor(mH/2.0+sinf(d/ripple_interval + call/((256.0-128.0)/20.0))*mH/2.0); //between 0 and 8

            leds[x + height * mW + z * mW * mH] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
        }
    }
  }
};

class SphereMove3DEffect:public Effect {
public:
  const char * name() {
    return "SphereMove 3D";
  }
  void setup() {} //not implemented yet
  void loop() {
    uint16_t origin_x, origin_y, origin_z, d;
    float diameter;

    fill_solid(leds, nrOfLeds, CRGB::Black);
    // fill(CRGB::Black);

    uint32_t interval = call/((256.0-128.0)/20.0);

    uint16_t mW = width;
    uint16_t mH = height;
    uint16_t mD = depth;

    origin_x = 3.5+sinf(interval)*2.5;
    origin_y = 3.5+cosf(interval)*2.5;
    origin_z = 3.5+cosf(interval)*2.0;

    diameter = 2.0+sinf(interval/3.0);

                    // CRGBPalette256 pal;
    for (int x=0; x<mW; x++) {
        for (int y=0; y<mH; y++) {
            for (int z=0; z<mD; z++) {
                d = distance(x, y, z, origin_x, origin_y, origin_z);

                if (d>diameter && d<diameter+1) {
                    leds[x + height * mW + z * mW * mH] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
                }
            }
        }
    }
  }
}; // 3DSphereMove


static std::vector<Effect *> effects;

class AppModLeds:public Module {

public:
  uint8_t dataPin = 16; 
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;

  AppModLeds() :Module("Leds") {};

  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);

    ui->initSlider(parentObject, "bri", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Brightness");
    }, [](JsonObject object) { //chFun
      bri = map(object["value"], 0, 100, 0, 255);
      FastLED.setBrightness(bri);
      print->print("Set Brightness to %d -> %d\n", object["value"].as<int>(), bri);
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

    ui->initCanvas(parentObject, "pview", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Preview");
    }, nullptr, [](JsonObject object, uint8_t* buffer) { //loopFun
      // send leds preview to clients
      for (size_t i = 0; i < buffer[0] * 256 + buffer[1]; i++)
      {
        buffer[i*3+4] = leds[i].red;
        buffer[i*3+4+1] = leds[i].green;
        buffer[i*3+4+2] = leds[i].blue;
      }
      //new values
      buffer[0] = width;
      buffer[1] = height;
      buffer[2] = depth;
      buffer[3] = max(nrOfLeds * web->ws->count()/200, 16U); //interval in ms * 10, not too fast
    });

    ui->initDropdown(parentObject, "ledFix", 0, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "LedFix");
      JsonArray lov = web->addResponseArray(object, "lov");
      files->dirToJson2(lov, "lf"); //only files containing lf
    }, [](JsonObject object) { //chFun
      print->print("%s Change %s to %d\n", "initDropdown chFun", object["id"].as<const char *>(), object["value"].as<int>());
      DynamicJsonDocument lmJson (8192);
      char fileName[30] = "/"; //add root prefix
      strcat(fileName, files->seqNrToName(object["value"].as<int>()));
      if (files->readFile(fileName)) {
        files->readObjectFromFile(fileName, &lmJson);
        print->printJDocInfo("ledFix", lmJson);
        print->printJson("ledFix", lmJson);

        width = lmJson["pview"]["json"]["width"];
        height = lmJson["pview"]["json"]["height"];
        depth = lmJson["pview"]["json"]["depth"];

        //change the setup based on the json file
        nrOfLeds = lmJson["pview"]["json"]["leds"].size() / (depth>1?3:2);
        if (nrOfLeds == 0) { //avoid div/0 in %nrOfLeds
          print->print("ledFix could not find nrofleds");
          nrOfLeds = 30;
        }
        // mdl->setValue("nrOfLeds", nrOfLeds); //done by width/height and depth already
        mdl->setValue("width", width);
        mdl->setValue("height", height);
        mdl->setValue("depth", depth);

        //send the json to pview...
        // JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->as<JsonVariant>();
        // (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->clear();
        // responseVariant["pview"]["json"] = lmJson;
        web->sendDataWs(lmJson);
        print->print("ledfix send ws done\n");
      }
    });

    ui->initNumber(parentObject, "width", width, [](JsonObject object) { //uiFun
      web->addResponseV(object, "comment", "Max %d", 256);
    }, [](JsonObject object) { //chFun
      width = object["value"];
      if (width>256) {width = 256;mdl->setValue("width", 256);};
      changeDimensions();
      fadeToBlackBy( leds, nrOfLeds, 100);
    });

    ui->initNumber(parentObject, "height", height, [](JsonObject object) { //uiFun
      web->addResponseV(object, "comment", "Max %d", 64);
    }, [](JsonObject object) { //chFun
      height = object["value"];
      if (height>64) {height = 64;mdl->setValue("height", 64);};
      changeDimensions();
      fadeToBlackBy( leds, nrOfLeds, 100);
    });

    ui->initNumber(parentObject, "depth", depth, [](JsonObject object) { //uiFun
      web->addResponseV(object, "comment", "Max %d", 16);
    }, [](JsonObject object) { //chFun
      depth = object["value"];
      if (depth>16) {depth = 16;mdl->setValue("depth", 16);};
      changeDimensions();
      fadeToBlackBy( leds, nrOfLeds, 100);
    });

    ui->initNumber(parentObject, "fps", fps, [](JsonObject object) { //uiFun
      web->addResponse(object, "comment", "Frames per second");
    }, [](JsonObject object) { //chFun
      fps = object["value"];
      print->print("fps changed %d\n", fps);
    });

    ui->initDisplay(parentObject, "realFps");

    ui->initDisplay(parentObject, "nrOfLeds", nullptr, [](JsonObject object) { //uiFun
      web->addResponseV(object, "comment", "Max %d", NUM_LEDS);
    });
    mdl->setValue("nrOfLeds", nrOfLeds); //set here as changeDimensions already called for width/height/depth

    ui->initNumber(parentObject, "dataPin", dataPin, [](JsonObject object) { //uiFun
      web->addResponseV(object, "comment", "Not implemented yet (fixed to %d)", DATA_PIN);
    }, [](JsonObject object) { //chFun
      print->print("Set data pin to %d\n", object["value"].as<int>());
    });

    ui->initDropdown(parentObject, "ledFixGen", 3, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Ledfix generator");
      JsonArray lov = web->addResponseArray(object, "lov");
      lov.add("R24"); //0
      lov.add("R35"); //1
      lov.add("M88"); //2
      lov.add("M885"); //3
    }, [](JsonObject object) { //chFun
      DynamicJsonDocument lmJson (8192);
      JsonObject lmObject = lmJson.to<JsonObject>();

      const char * name = "TT";
      uint16_t nrOfLeds = 30; //default
      uint16_t diameter = 100; //in mm

      uint16_t width = 0;
      uint8_t height = 0;
      uint8_t depth = 0;

      size_t fix = object["value"];
      switch (fix) {
        case 0: //R24
        case 1: //R35
          diameter = 100; //in mm

          if (fix == 0) {
            name = "R24";
            nrOfLeds = 24;
          }
          else {
            name = "R35";
            nrOfLeds = 35;
          }

          lmObject["pview"]["json"]["name"] = name;

          lmObject["pview"]["json"]["scale"] = "mm";
          lmObject["pview"]["json"]["size"] = diameter;
          lmObject["pview"]["json"]["pin"] = 16;
          lmObject["pview"]["json"]["leds"] = lmJson.createNestedArray();

          for (int i=0; i<nrOfLeds; i++) {
            float radians = i*360/nrOfLeds * (M_PI / 180);
            uint16_t x = diameter/2 * (1 + sinf(radians));
            uint8_t y = diameter/2 * (1+ cosf(radians));
            JsonArray array2 = lmObject["pview"]["json"]["leds"].createNestedArray();
            array2.add(x);
            array2.add(y);
            width = max(width, x);
            height = max(height, y);
            depth = 1;

          }
          width = 10; //overrule wip
          height = 10;
          depth = 1;
          break;
        case 2: //M88
          name = "M88";
          diameter = 8; //in cm
          nrOfLeds = 64;

          lmObject["pview"]["json"]["name"] = name;
          lmObject["pview"]["json"]["scale"] = "cm";
          lmObject["pview"]["json"]["size"] = diameter;
          lmObject["pview"]["json"]["pin"] = 16;
          lmObject["pview"]["json"]["leds"] = lmJson.createNestedArray();

          for (uint8_t y = 0; y<8; y++)
            for (uint16_t x = 0; x<8 ; x++) {
              JsonArray array2 = lmObject["pview"]["json"]["leds"].createNestedArray();
              array2.add(x);
              array2.add(y);
              width = max(width, x);
              height = max(height, y);
              depth = 1;
            }

          width = 8; //overrule wip
          height = 8;
          depth = 1;
          break;
        case 3: //M885
          name = "M885";
          diameter = 80; //in mm
          nrOfLeds = 64*8;

          lmObject["pview"]["json"]["name"] = name;
          lmObject["pview"]["json"]["scale"] = "cm";
          lmObject["pview"]["json"]["size"] = diameter;
          lmObject["pview"]["json"]["pin"] = 16;
          lmObject["pview"]["json"]["leds"] = lmJson.createNestedArray();

          for (uint8_t z = 0; z<8; z++)
            for (uint8_t y = 0; y<8; y++)
              for (uint16_t x = 0; x<8 ; x++) {
                JsonArray array2 = lmObject["pview"]["json"]["leds"].createNestedArray();
                array2.add(x);
                array2.add(y);
                array2.add(z);
                width = max(width, x);
                height = max(height, y);
                depth = max(depth, z);
              }

          width = 8; //overrule wip
          height = 8;
          depth = 8;
          break;
      }

      lmObject["pview"]["json"]["width"] = width;
      lmObject["pview"]["json"]["height"] = height;
      lmObject["pview"]["json"]["depth"] = depth;

      char fileName[30] = "/lf";
      strcat(fileName, name);
      strcat(fileName, ".json");
      print->printJDocInfo("ledFixGen", lmJson);
      files->writeObjectToFile(fileName, &lmJson);
      print->printJson(fileName, lmJson);

      //reload ledfix dropdown
      ui->processUiFun("ledFix");
    });

    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new RunningEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new JuggleEffect);
    effects.push_back(new Ripples3DEffect);
    effects.push_back(new SphereMove3DEffect);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    if(millis() - frameMillis >= 1000.0/fps) {
      frameMillis = millis();

      Effect* effect = effects[mdl->getValue("fx")];
      effect->loop();

      // yield();
      FastLED.show();  

      frameCounter++;
      call++;
    }
    if (millis() - secondMillis >= 1000 || !secondMillis) {
      secondMillis = millis();
      mdl->setValueV("realFps", "%lu /s", frameCounter);
      frameCounter = 0;
    }

    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }

  static void changeDimensions() { //static because of lambda functions
    nrOfLeds = min(width * height * depth, NUM_LEDS); //not highter then 4K leds
    mdl->setValue("nrOfLeds", nrOfLeds);
    print->print("changeDimensions %d x %d x %d = %d\n", width, height, depth, nrOfLeds);
    // if (!leds)
    //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
    // else
    //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
    // if (leds) free(leds);
    // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
    // leds = (CRGB*)reallocarray
    // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 
  }

};

static AppModLeds *lds;