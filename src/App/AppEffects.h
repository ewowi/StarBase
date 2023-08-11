/*
   @title     StarMod
   @file      AppEffects.h
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static unsigned long call = 0;

//should not contain variables/bytes to keep mem as small as possible!!
class Effect {
public:
  virtual const char * name() { return nullptr;}

  virtual void setup() {}

  virtual void loop() {
    call++;

    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }

  virtual bool controls(JsonObject parentVar) {return false;}
};

class RainbowEffect: public Effect {
public:
  const char * name() {
    return "Rainbow 1D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
    // FastLED's built-in rainbow generator
    fill_rainbow( ledsP, LedsV::nrOfLedsP, gHue, 7);
  }
};

class RainbowWithGlitterEffect:public RainbowEffect {
public:
  const char * name() {
    return "Rainbow with glitter 1D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
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

class SinelonEffect: public Effect {
public:
  const char * name() {
    return "Sinelon 1D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 20);
    int pos = beatsin16( 13, 0, LedsV::nrOfLedsV-1 );
    // ledsV[pos] += CHSV( gHue, 255, 192);
    ledsV[pos] = ledsV.getPixelColor(pos) + CHSV( gHue, 255, 192);
    // CRGB x = ledsV[pos];
  }
};

class RunningEffect: public Effect {
public:
  const char * name() {
    return "Running 1D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 70); //physical leds
    // int pos0 = (call-1)%ledsV.nrOfLeds;
    // leds[pos0] = CHSV( 0,0,0);
    int pos = call%LedsV::nrOfLedsV; //Virtual leds
    ledsV[pos] = CHSV( gHue, 255, 192); //make sore the right physical leds get their value
  }
};

class ConfettiEffect: public Effect {
public:
  const char * name() {
    return "Confetti 1D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 10);
    int pos = random16(LedsV::nrOfLedsP);
    ledsP[pos] += CHSV( gHue + random8(64), 200, 255);
  }
};

class BPMEffect: public Effect {
public:
  const char * name() {
    return "Beats per minute 1D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < LedsV::nrOfLedsV; i++) { //9948
      ledsV[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
    }
  }
  bool controls(JsonObject parentVar) {
    return false;
  }
};

class JuggleEffect: public Effect {
public:
  const char * name() {
    return "Juggle 1D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 20);
    uint8_t dothue = 0;
    for( int i = 0; i < 8; i++) {
      ledsP[beatsin16( i+7, 0, LedsV::nrOfLedsP-1 )] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
};

class Ripples3DEffect: public Effect {
public:
  const char * name() {
    return "Ripples 3D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
    uint8_t interval = mdl->getValue("interval");

    float ripple_interval = 1.3 * (interval/128.0);

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
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "interval", 128, false);
    return true;
  }
};

class SphereMove3DEffect: public Effect {
public:
  const char * name() {
    return "SphereMove 3D";
  }
  void setup() {}
  void loop() {
    Effect::loop();
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

//XY used by blur2d
uint16_t XY( uint8_t x, uint8_t y) {
  return x + y * LedsV::widthV;
}

//Frizzles2D inspired by WLED, Stepko, Andrew Tuline, https://editor.soulmatelights.com/gallery/640-color-frizzles
class Frizzles2D: public Effect {
public:
  const char * name() {
    return "Frizzles 2D";
  }

  void setup() {
    // fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 100); //like more the gradual change
  }

  void loop() {
    Effect::loop();
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 16);
    CRGBPalette16 palette = PartyColors_p;

    for (size_t i = 8; i > 0; i--) {
      uint8_t x = beatsin8(mdl->getValue("speed").as<int>()/8 + i, 0, LedsV::widthV - 1);
      uint8_t y = beatsin8(mdl->getValue("intensity").as<int>()/8 - i, 0, LedsV::heightV - 1);
      CRGB color = ColorFromPalette(palette, beatsin8(12, 0, 255), 255);
      ledsV[x + y * LedsV::widthV] = color;
    }
    blur2d(ledsP, LedsV::widthP, LedsV::heightP, mdl->getValue("blur")); //this is tricky as FastLed is not aware of our virtual 
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", 128, false);
    ui->initSlider(parentVar, "intensity", 128, false);
    ui->initSlider(parentVar, "blur", 128, false);
    return true;
  }
}; // Frizzles2D

class Lines2D: public Effect {
public:
  const char * name() {
    return "Lines 2D";
  }

  void setup() {}

  void loop() {
    Effect::loop();
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 100);
    CRGBPalette16 palette = PartyColors_p;

    if (mdl->getValue("Vertical").as<bool>()) {
      size_t x = call%LedsV::widthV;
      for (size_t y = 0; y <  LedsV::heightV; y++) {
        ledsV[x + y * LedsV::widthV] = CHSV( gHue, 255, 192);
      }
    } else {
      size_t y = call%LedsV::heightV;
      for (size_t x = 0; x <  LedsV::widthV; x++) {
        ledsV[x + y * LedsV::widthV] = CHSV( gHue, 255, 192);
      }
    }
  }

  bool controls(JsonObject parentVar) {
    ui->initCheckBox(parentVar, "Vertical", false, false);
    return true;
  }
}; // Lines2D

uint8_t gamma8(uint8_t b) { //we do nothing with gamma for now
  return b;
}

//DistortionWaves2D  inspired by WLED, ldirko, https://editor.soulmatelights.com/gallery/1089-distorsion-waves
class DistortionWaves2D: public Effect {
public:
  const char * name() {
    return "DistortionWaves 2D";
  }

  void setup() {
    // fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 100); //like more the gradual change
  }

  void loop() {
    Effect::loop();

    const uint16_t cols = LedsV::widthV;
    const uint16_t rows = LedsV::widthV;

    uint8_t speed = mdl->getValue("speed").as<int>()/32;
    uint8_t scale = mdl->getValue("scale").as<int>()/32;

    uint8_t  w = 2;

    uint16_t a  = millis()/32;
    uint16_t a2 = a/2;
    uint16_t a3 = a/3;

    uint16_t cx =  beatsin8(10-speed,0,cols-1)*scale;
    uint16_t cy =  beatsin8(12-speed,0,rows-1)*scale;
    uint16_t cx1 = beatsin8(13-speed,0,cols-1)*scale;
    uint16_t cy1 = beatsin8(15-speed,0,rows-1)*scale;
    uint16_t cx2 = beatsin8(17-speed,0,cols-1)*scale;
    uint16_t cy2 = beatsin8(14-speed,0,rows-1)*scale;
    
    uint16_t xoffs = 0;
    for (int x = 0; x < cols; x++) {
      xoffs += scale;
      uint16_t yoffs = 0;

      for (int y = 0; y < rows; y++) {
        yoffs += scale;

        byte rdistort = cos8((cos8(((x<<3)+a )&255)+cos8(((y<<3)-a2)&255)+a3   )&255)>>1; 
        byte gdistort = cos8((cos8(((x<<3)-a2)&255)+cos8(((y<<3)+a3)&255)+a+32 )&255)>>1; 
        byte bdistort = cos8((cos8(((x<<3)+a3)&255)+cos8(((y<<3)-a) &255)+a2+64)&255)>>1; 

        byte valueR = rdistort+ w*  (a- ( ((xoffs - cx)  * (xoffs - cx)  + (yoffs - cy)  * (yoffs - cy))>>7  ));
        byte valueG = gdistort+ w*  (a2-( ((xoffs - cx1) * (xoffs - cx1) + (yoffs - cy1) * (yoffs - cy1))>>7 ));
        byte valueB = bdistort+ w*  (a3-( ((xoffs - cx2) * (xoffs - cx2) + (yoffs - cy2) * (yoffs - cy2))>>7 ));

        valueR = gamma8(cos8(valueR));
        valueG = gamma8(cos8(valueG));
        valueB = gamma8(cos8(valueB));

        ledsV[x + y * LedsV::widthV] = CRGB(valueR, valueG, valueB);
      }
    }
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", 128, false);
    ui->initSlider(parentVar, "scale", 128, false);
    return true;
  }
}; // DistortionWaves2D


//BouncingBalls1D  inspired by WLED
//each needs 12 bytes
typedef struct Ball {
  unsigned long lastBounceTime;
  float impactVelocity;
  float height;
} ball;

#define maxNumBalls 16

class BouncingBalls1D: public Effect {
public:
  Ball balls[maxNumBalls];

  const char * name() {
    return "Bouncing Balls 1D";
  }

  void setup() {
    for (size_t i = 0; i < maxNumBalls; i++) balls[i].lastBounceTime = millis();
  }

  void loop() {
    Effect::loop();

    uint8_t grav = mdl->getValue("gravity");
    uint8_t nrOfBalls = mdl->getValue("nrOfBalls");

    // number of balls based on intensity setting to max of 7 (cycles colors)
    // non-chosen color is a random color
    uint16_t numBalls = (grav * (maxNumBalls - 1)) / 255 + 1; // minimum 1 ball
    const float gravity = -9.81f; // standard value of gravity
    // const bool hasCol2 = SEGCOLOR(2);
    const unsigned long time = millis();

    for (size_t i = 0; i < numBalls; i++) {
      float timeSinceLastBounce = (time - balls[i].lastBounceTime)/((255-grav)/64 +1);
      float timeSec = timeSinceLastBounce/1000.0f;
      balls[i].height = (0.5f * gravity * timeSec + balls[i].impactVelocity) * timeSec; // avoid use pow(x, 2) - its extremely slow !

      if (balls[i].height <= 0.0f) {
        balls[i].height = 0.0f;
        //damping for better effect using multiple balls
        float dampening = 0.9f - float(i)/float(numBalls * numBalls); // avoid use pow(x, 2) - its extremely slow !
        balls[i].impactVelocity = dampening * balls[i].impactVelocity;
        balls[i].lastBounceTime = time;

        if (balls[i].impactVelocity < 0.015f) {
          float impactVelocityStart = sqrtf(-2.0f * gravity) * random8(5,11)/10.0f; // randomize impact velocity
          balls[i].impactVelocity = impactVelocityStart;
        }
      } else if (balls[i].height > 1.0f) {
        continue; // do not draw OOB ball
      }

      // uint32_t color = SEGCOLOR(0);
      // if (SEGMENT.palette) {
      //   color = SEGMENT.color_wheel(i*(256/MAX(numBalls, 8)));
      // } 
      // else if (hasCol2) {
      //   color = SEGCOLOR(i % NUM_COLORS);
      // }

      int pos = roundf(balls[i].height * (LedsV::nrOfLedsV - 1));

      CRGBPalette16 palette = PartyColors_p;

      CRGB color = ColorFromPalette(palette, i*(256/max(numBalls, (uint16_t)8)), 255);

      ledsV[pos] = color;
      // if (SEGLEN<32) SEGMENT.setPixelColor(indexToVStrip(pos, stripNr), color); // encode virtual strip into index
      // else           SEGMENT.setPixelColor(balls[i].height + (stripNr+1)*10.0f, color);
    } //nrOfBalls
  }

  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "gravity", 128, false);
    ui->initSlider(parentVar, "nrOfBalls", 128, false);
    return true;
  }
}; // DistortionWaves2D

static std::vector<Effect *> effects;