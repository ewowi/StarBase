#include "FastLED.h"

#define DATA_PIN 16
#define NUM_LEDS_Fastled 1024
#define NUM_LEDS_preview 2000

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

// CRGB *leds = nullptr;
CRGB leds[NUM_LEDS_preview];
static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static uint16_t nrOfLeds = 64; 
static uint16_t width = 8; 
static uint16_t height = 8; 
static uint16_t depth = 1; 
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

    parentObject = ui->initModule(parentObject, name);

    ui->initSlider(parentObject, "bri", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
      web->addResponse(object["id"], "label", "Brightness");
    }, [](JsonObject object) { //chFun
      bri = map(object["value"], 0, 100, 0, 255);
      FastLED.setBrightness(bri);
      print->print("Set Brightness to %d -> %d\n", object["value"].as<int>(), bri);
    });

    ui->initSelect(parentObject, "fx", 6, false, [](JsonObject object) { //uiFun. 6: Juggles is default
      web->addResponse(object["id"], "label", "Effect");
      web->addResponse(object["id"], "comment", "Effect to show");
      JsonArray lov = web->addResponseA(object["id"], "lov");
      for (Effect *effect:effects) {
        lov.add(effect->name());
      }
    }, [](JsonObject object) { //chFun
      print->print("%s Change %s to %d\n", "initSelect chFun", object["id"].as<const char *>(), object["value"].as<int>());
    });

    ui->initCanvas(parentObject, "pview", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
      web->addResponse(object["id"], "label", "Preview");
      // web->addResponse(object["id"], "comment", "Click to enlarge");
    }, nullptr, [](JsonObject object, uint8_t* buffer) { //loopFun
      // send leds preview to clients
      for (size_t i = 0; i < buffer[0] * 256 + buffer[1]; i++)
      {
        buffer[i*3+4] = leds[i].red;
        buffer[i*3+4+1] = leds[i].green;
        buffer[i*3+4+2] = leds[i].blue;
      }
      //new values
      buffer[0] = nrOfLeds/256;
      buffer[1] = nrOfLeds%256;
      buffer[3] = max(nrOfLeds * web->ws->count()/200, 16U); //interval in ms * 10, not too fast
    });

    ui->initSelect(parentObject, "ledFix", 0, false, [](JsonObject object) { //uiFun
      web->addResponse(object["id"], "label", "LedFix");
      JsonArray lov = web->addResponseA(object["id"], "lov");
      files->dirToJson(lov, true, "lf"); //only files containing lf, alphabetically

      // ui needs to load the file also initially
      char fileName[30] = "";
      if (files->seqNrToName(fileName, object["value"])) {
        web->addResponse("pview", "file", fileName);
      }
    }, [](JsonObject object) { //chFun
      print->print("%s Change %s to %d\n", "initSelect chFun", object["id"].as<const char *>(), object["value"].as<int>());

      char fileName[30] = "";
      if (files->seqNrToName(fileName, object["value"])) {
        LazyJsonRDWS ljrdws(fileName); //open fileName for deserialize

        //what to deserialize
        ljrdws.lookFor("width", &width);
        ljrdws.lookFor("height", &height);
        ljrdws.lookFor("depth", &depth);
        ljrdws.lookFor("nrOfLeds", &nrOfLeds);

        if (ljrdws.deserialize()) {
          print->print("ljrdws whd %d %d %d and %d\n", width, height, depth, nrOfLeds);

          mdl->setValueI("width", width);
          mdl->setValueI("height", height);
          mdl->setValueI("depth", depth);
          mdl->setValueI("nrOfLeds", nrOfLeds);

          //send to pview a message to get file filename
          JsonVariant responseVariant = (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->as<JsonVariant>();
          (strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) != 0?web->responseDoc0:web->responseDoc1)->clear();
          web->addResponse("pview", "file", fileName);
          web->sendDataWs(responseVariant);
          print->printJson("ledfix chFun send ws done", responseVariant); //during server startup this is not send to a client, so client refresh should also trigger this
        } // if deserialize
      } //if fileName
    });

    ui->initText(parentObject, "width", nullptr, true, [](JsonObject object) { //uiFun
      web->addResponseV(object["id"], "comment", "Max %dK", 32);
    });

    ui->initText(parentObject, "height", nullptr, true, [](JsonObject object) { //uiFun
      web->addResponseV(object["id"], "comment", "Max %dK", 32);
    });

    ui->initText(parentObject, "depth", nullptr, true, [](JsonObject object) { //uiFun
      web->addResponseV(object["id"], "comment", "Max %dK", 32);
    });

    ui->initText(parentObject, "nrOfLeds", nullptr, true, [](JsonObject object) { //uiFun
      web->addResponseV(object["id"], "comment", "Max %d (FastLed max %d)", NUM_LEDS_preview, NUM_LEDS_Fastled);
    });
    // mdl->setValueI("nrOfLeds", nrOfLeds); //set here as changeDimensions already called for width/height/depth

    // if (!leds)
    //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
    // else
    //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
    // if (leds) free(leds);
    // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
    // leds = (CRGB*)reallocarray

    ui->initNumber(parentObject, "fps", fps, [](JsonObject object) { //uiFun
      web->addResponse(object["id"], "comment", "Frames per second");
    }, [](JsonObject object) { //chFun
      fps = object["value"];
      print->print("fps changed %d\n", fps);
    });

    ui->initText(parentObject, "realFps");

    ui->initNumber(parentObject, "dataPin", dataPin, [](JsonObject object) { //uiFun
      web->addResponseV(object["id"], "comment", "Not implemented yet (fixed to %d)", DATA_PIN);
    }, [](JsonObject object) { //chFun
      print->print("Set data pin to %d\n", object["value"].as<int>());
    });

    enum Fixtures
    {
      Spiral,
      R24,
      R35,
      M88,
      C888,
      C885,
      HSC,
      Globe
    };

    ui->initSelect(parentObject, "ledFixGen", 3, false, [](JsonObject object) { //uiFun
      web->addResponse(object["id"], "label", "Ledfix generator");
      JsonArray lov = web->addResponseA(object["id"], "lov");
      lov.add("Spiral"); //0
      lov.add("R24"); //1
      lov.add("R35"); //2
      lov.add("M88"); //3
      lov.add("C888"); //4
      lov.add("C885"); //5
      lov.add("HSC"); //6
      lov.add("Globe"); //7
    }, [](JsonObject object) { //chFun

      const char * name = "TT";
      uint16_t nrOfLeds = 64; //default
      uint16_t diameter = 100; //in mm

      uint16_t width = 0;
      uint8_t height = 0;
      uint8_t depth = 0;

      size_t fix = object["value"];
      switch (fix) {
        case Spiral:
          name = "Spiral";
          nrOfLeds = 64;
          break;
        case R24:
          name = "R24";
          nrOfLeds = 24;
          break;
        case R35:
          name = "R35";
          nrOfLeds = 35;
          break;
        case M88:
          name = "M88";
          nrOfLeds = 64;
          break;
        case C888:
          name = "C888";
          nrOfLeds = 512;
          break;
        case C885:
          name = "C885";
          nrOfLeds = 320;
          break;
        case HSC:
          name = "HSC";
          nrOfLeds = 2000;
          break;
        case Globe:
          name = "Globe";
          nrOfLeds = 512;
          break;
      }

      char fileName[30] = "/lf";
      strcat(fileName, name);
      strcat(fileName, ".json");

      File f = files->open(fileName, "w");
      if (f) {
        f.print("{"); //{\"pview\":{\"json\":
      }
      else
        print->print("ledFixGen Could not open file %s for writing\n", fileName);
      
      char sep[3]="";
      char sep2[3]="";

      uint8_t pin = 10;

      f.printf("\"name\":\"%s\"", name);
      f.printf(",\"nrOfLeds\":%d", nrOfLeds);
      // f.printf(",\"pin\":%d",16);
      switch (fix) {
        case Spiral:
        case R24:
        case R35:
          diameter = 100; //in mm

          f.printf(",\"scale\":\"%s\"", "mm");
          f.printf(",\"size\":%d", diameter);

          width = 10;
          height = 10;
          depth = 1;
          f.printf(",\"width\":%d", width);
          f.printf(",\"height\":%d", height);
          f.printf(",\"depth\":%d", depth);

          f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
          strcpy(sep, "");
          for (int i=0; i<nrOfLeds; i++) {
            float radians = i*360/nrOfLeds * (M_PI / 180);
            uint16_t x = diameter/2 * (1 + sinf(radians));
            uint8_t y = diameter/2 * (1+ cosf(radians));
            f.printf("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
          }
          f.printf("]}]");
          break;
        case M88:
          diameter = 8; //in cm

          f.printf(",\"scale\":\"%s\"", "cm");
          f.printf(",\"size\":%d", diameter);

          width = 8;
          height = 8;
          depth = 1;
          f.printf(",\"width\":%d", width);
          f.printf(",\"height\":%d", height);
          f.printf(",\"depth\":%d", depth);

          f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
          strcpy(sep,"");
          for (uint8_t y = 0; y<8; y++)
            for (uint16_t x = 0; x<8 ; x++) {
              // width = max(width, x);
              // height = max(height, y);
              // depth = 1;
              f.printf("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
            }
          f.printf("]}]");

          break;
        case C888:
          diameter = 8; //in cm

          f.printf(",\"scale\":\"%s\"", "cm");
          f.printf(",\"size\":%d", diameter);

          width = 8;
          height = 8;
          depth = 8;
          f.printf(",\"width\":%d", width);
          f.printf(",\"height\":%d", height);
          f.printf(",\"depth\":%d", depth);

          f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
          strcpy(sep,"");
          for (uint8_t z = 0; z<depth; z++)
            for (uint8_t y = 0; y<height; y++)
              for (uint16_t x = 0; x<width ; x++) {
                f.printf("%s[%d,%d,%d]", sep, x,y,z); strcpy(sep, ",");
              }
          f.printf("]}]");

          break;
        case C885:
        case HSC:
          if (fix==5) {
            diameter = 8; //in cm
            width = 8;
            height = 8;
            depth = 8;
            f.printf(",\"scale\":\"%s\"", "cm");
          }
          else {
            diameter = 20; //in dm
            width = 20;
            height = 20;
            depth = 20;
            f.printf(",\"scale\":\"%s\"", "dm");
          }

          f.printf(",\"size\":%d", diameter);

          f.printf(",\"width\":%d", width);
          f.printf(",\"height\":%d", height);
          f.printf(",\"depth\":%d", depth);

          f.printf(",\"outputs\":[");
          strcpy(sep,"");

          //front and back
          for (uint8_t z = 0; z<depth; z+=depth-1) {
            f.printf("%s{\"pin\":%d,\"leds\":[", sep, pin++);strcpy(sep, ",");
            strcpy(sep2,"");
            for (uint8_t y = 0; y<height; y++)
              for (uint16_t x = 0; x<width ; x++) {
                f.printf("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
              }
            f.printf("]}");
          }
          //NO botom and top
          for (uint8_t y = height-1; y<height; y+=height-1) {
            f.printf("%s{\"pin\":%d,\"leds\":[", sep, pin++);strcpy(sep, ",");
            strcpy(sep2,"");
            for (uint8_t z = 0; z<depth; z++)
              for (uint16_t x = 0; x<width ; x++) {
                f.printf("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
              }
            f.printf("]}");
          }

          //left and right
          for (uint16_t x = 0; x<width ; x+=width-1) {
            f.printf("%s{\"pin\":%d,\"leds\":[", sep, pin++);strcpy(sep, ",");
            strcpy(sep2,"");
            for (uint8_t z = 0; z<depth; z++)
              for (uint8_t y = 0; y<height; y++) {
                f.printf("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
              }
            f.printf("]}");
          }
       
          f.printf("]");

          break;
        case Globe:
          diameter = 100; //in mm

          f.printf(",\"scale\":\"%s\"", "mm");
          f.printf(",\"size\":%d", diameter);

          width = 10;
          height = 10;
          depth = 10;
          f.printf(",\"width\":%d", width);
          f.printf(",\"height\":%d", height);
          f.printf(",\"depth\":%d", depth);

          f.printf(",\"leds\":[");
          strcpy(sep, "");
          for (int i=0; i<nrOfLeds; i++) {
            float radians = i*360/nrOfLeds * (M_PI / 180);
            uint16_t x = diameter/2 * (1 + sinf(radians));
            uint8_t y = diameter/2 * (1+ cosf(radians));
            uint8_t z = diameter/2 * (1+ cosf(radians));
            f.printf("%s[%d,%d,%d]", sep, x,y,z); strcpy(sep, ",");

          }
          f.printf("]");
          break;
      }

      f.print("}"); //}}
      f.close();

      files->filesChange();

      //reload ledfix select
      ui->processUiFun("ledFix");
    }); //ledFixGen

    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new RunningEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new JuggleEffect);
    effects.push_back(new Ripples3DEffect);
    effects.push_back(new SphereMove3DEffect);

    // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS_Fastled); 

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
};

static AppModLeds *lds;