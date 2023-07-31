/*
   @title     StarMod
   @file      AppEffects.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static unsigned long call = 0;

//should not contain variables/bytes to keep mem as small as possible!!
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
    fill_rainbow( ledsP, LedsV::nrOfLedsP, gHue, 7);
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
      ledsP[ random16(LedsV::nrOfLedsP) ] += CRGB::White;
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
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 20);
    int pos = beatsin16( 13, 0, LedsV::nrOfLedsV-1 );
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
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 70); //physical leds
    // int pos0 = (call-1)%ledsV.nrOfLeds;
    // leds[pos0] = CHSV( 0,0,0);
    int pos = call%LedsV::nrOfLedsV; //Virtual leds
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
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 10);
    int pos = random16(LedsV::nrOfLedsP);
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
    for( int i = 0; i < LedsV::nrOfLedsV; i++) { //9948
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
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 20);
    uint8_t dothue = 0;
    for( int i = 0; i < 8; i++) {
      ledsP[beatsin16( i+7, 0, LedsV::nrOfLedsP-1 )] |= CHSV(dothue, 200, 255);
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

    fill_solid(ledsP, LedsV::nrOfLedsP, CRGB::Black);
    // fill(CRGB::Black);

    uint16_t mW = LedsV::widthV;
    uint16_t mH = LedsV::heightV;
    uint16_t mD = LedsV::depthV;

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

    fill_solid(ledsP, LedsV::nrOfLedsP, CRGB::Black);
    // fill(CRGB::Black);

    uint32_t interval = call/((256.0-128.0)/20.0);

    uint16_t mW = LedsV::widthV;
    uint16_t mH = LedsV::heightV;
    uint16_t mD = LedsV::depthV;

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
                    ledsV[x + LedsV::heightV * mW + z * mW * mH] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
                }
            }
        }
    }
  }
}; // SphereMove3DEffect

class Frizzles2D:public Effect {
public:
  const char * name() {
    return "Frizzles2D";
  }
  void setup() {} //not implemented yet
  void loop() {
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 16);
    CRGBPalette16 palette = PartyColors_p;

    for (size_t i = 8; i > 0; i--) {
      uint8_t x = beatsin8(128/8 + i, 0, LedsV::widthV - 1);
      uint8_t y = beatsin8(128/8 - i, 0, LedsV::heightV - 1);
      CRGB color = ColorFromPalette(palette, beatsin8(12, 0, 255), 255);
      ledsV[x + y * LedsV::widthV] = color;
    }
    // blur2d(ledsP, LedsV::width, LedsV::height, 255);
    // SEGMENT.blur(SEGMENT.custom1>>3);

  }
}; // Frizzles2D

static std::vector<Effect *> effects;