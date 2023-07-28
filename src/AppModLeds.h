#pragma once
#include "Module.h"

#include <vector>
#include "FastLED.h"

#define DATA_PIN 16
#define NUM_LEDS_FastLed 1024
#define NUM_LEDS_Preview 2000

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

class LedsV {

public:
  // CRGB *leds = nullptr;
  CRGB ledsP[NUM_LEDS_Preview];

  std::vector<std::vector<uint16_t>> mappingTable;
  uint16_t mappingTableLedCounter = 0;

  void ledFixProjectAndMap(JsonObject ledFixObject, JsonObject projectionObject);

  uint16_t indexVLocal = 0;

  // ledsV[indexV] stores indexV locally
  LedsV& operator[](uint16_t indexV);

  // CRGB& operator[](uint16_t indexV) {
  //   // indexVLocal = indexV;
  //   CRGB x = getPixelColor(indexV);
  //   return x;
  // }

  // ledsV = uses locally stored indexV and color to call setPixelColor
  LedsV& operator=(const CRGB color);

  // maps the virtual led to the physical led(s) and assign a color to it
  void setPixelColor(int indexV, CRGB color);

  CRGB getPixelColor(int indexV);

  // LedsV& operator+=(const CRGB color) {
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
  //   return *this;
  // }
  // LedsV& operator|=(const CRGB color) {
  //   // setPixelColor(indexVLocal, color);
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) | color);
  //   return *this;
  // }

  // LedsV& operator+(const CRGB color) {
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
  //   return *this;
  // }

};

//after header split they all needs to be static otherwise multiple definition link error
static LedsV ledsV = LedsV(); //virtual leds
static CRGB *ledsP = ledsV.ledsP; //physical leds, used by FastLed in particular

static float distance(uint16_t x1, uint16_t y1, uint16_t z1, uint16_t x2, uint16_t y2, uint16_t z2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
}

static uint16_t nrOfLedsP = 64; //amount of physical leds
static uint16_t nrOfLedsV = 64;  //amount of virtual leds (calculated by projection)

static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static uint16_t width = 8; 
static uint16_t height = 8; 
static uint16_t depth = 1; 
static uint16_t fps = 40;
static unsigned long call = 0;
static uint8_t bri = 10;


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
    fill_rainbow( ledsP, nrOfLedsP, gHue, 7);
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
      ledsP[ random16(nrOfLedsP) ] += CRGB::White;
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
    fadeToBlackBy( ledsP, nrOfLedsP, 20);
    int pos = beatsin16( 13, 0, nrOfLedsV-1 );
    // ledsV[pos] += CHSV( gHue, 255, 192);
    ledsV[pos] = ledsV.getPixelColor(pos) + CHSV( gHue, 255, 192);
    // CRGB x = ledsV[pos];
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
    fadeToBlackBy( ledsP, nrOfLedsP, 70); //physical leds
    // int pos0 = (call-1)%nrOfLeds;
    // leds[pos0] = CHSV( 0,0,0);
    int pos = call%nrOfLedsV; //Virtual leds
    ledsV[pos] = CHSV( gHue, 255, 192); //make sore the right physical leds get their value
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
    fadeToBlackBy( ledsP, nrOfLedsP, 10);
    int pos = random16(nrOfLedsP);
    ledsP[pos] += CHSV( gHue + random8(64), 200, 255);
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
    for( int i = 0; i < nrOfLedsV; i++) { //9948
      ledsV[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
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
    fadeToBlackBy( ledsP, nrOfLedsP, 20);
    uint8_t dothue = 0;
    for( int i = 0; i < 8; i++) {
      ledsP[beatsin16( i+7, 0, nrOfLedsP-1 )] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
};

class Ripples3DEffect:public Effect {
public:
  const char * name() {
    return "Ripples 3D";
  }
  void setup() {} //not implemented yet
  void loop() {
    float ripple_interval = 1.3;// * (SEGMENT.intensity/128.0);

    fill_solid(ledsP, nrOfLedsP, CRGB::Black);
    // fill(CRGB::Black);

    uint16_t mW = width;
    uint16_t mH = height;
    uint16_t mD = depth;

    for (int z=0; z<mD; z++) {
        for (int x=0; x<mW; x++) {
            float d = distance(3.5, 3.5, 0, x, z, 0)/9.899495*mH;
            uint16_t height = floor(mH/2.0+sinf(d/ripple_interval + call/((256.0-128.0)/20.0))*mH/2.0); //between 0 and 8

            ledsV[x + height * mW + z * mW * mH] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
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

    fill_solid(ledsP, nrOfLedsP, CRGB::Black);
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
                    ledsV[x + height * mW + z * mW * mH] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
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

  AppModLeds();

  void setup();

  void loop();
};

static AppModLeds *lds;