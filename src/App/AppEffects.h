/*
   @title     StarMod
   @file      AppEffects.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/
#ifdef USERMOD_WLEDAUDIO
  #include "User/UserModWLEDAudio.h"
#endif
#ifdef USERMOD_E131
  #include "../User/UserModE131.h"
#endif


static uint8_t gHue = 0; // rotating "base color" used by many of the patterns
static unsigned long call = 0;
static unsigned long step = 0;

//should not contain variables/bytes to keep mem as small as possible!!
class Effect {
public:
  virtual const char * name() { return nullptr;}
  virtual void setup() {}
  virtual void loop() {}
  virtual bool parameters(JsonObject parentVar) {return false;}
};

class RainbowEffect:public Effect {
public:
  const char * name() {
    return "Rainbow";
  }
  void setup() {}
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
  void setup() {}
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
  void setup() {}
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
  void setup() {}
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
  void setup() {}
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
  void setup() {}
  void loop() {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < LedsV::nrOfLedsV; i++) { //9948
      ledsV[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
    }
  }
  bool parameters(JsonObject parentVar) {
    return false;
  }
};

class JuggleEffect:public Effect {
public:
  const char * name() {
    return "Juggle";
  }
  void setup() {}
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
  void setup() {}
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
  void setup() {}
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
  void setup() {}
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

class RingEffect:public Effect {
  protected:
    uint8_t ringMap[9][2] = { // If you ring goes from ouside in, then reverse the order of this array
      {0, 0},     //0 Center Point
      {1, 8},     //1
      {9, 20},   //2
      {21, 36},   //3
      {37, 60},   //4
      {61, 92},   //5
      {93, 132},  //6
      {133, 180}, //7
      {181, 240}, //8 Outer Ring
    };
    CRGBPalette16 palette = PartyColors_p;
    bool INWARD; // TODO: param
    const static int RINGS = 9;
    uint8_t hue[RINGS];

    void setRing(int ring, CRGB colour) {
      for (int i = ringMap[ring][0]; i <= ringMap[ring][1]; i++) {
        ledsV[i] = colour;
      }
    }

};

class RingRandomFlow:public RingEffect {
public:
  const char * name() {
    return "RingRandomFlow";
  }
  void setup() {}
  void loop() {
    hue[0] = random(0, 255);
    for (int r = 0; r < RINGS; r++) {
      setRing(r, CHSV(hue[r], 255, 255));
    }
    for (int r = (RINGS - 1); r >= 1; r--) {
      hue[r] = hue[(r - 1)]; // set this ruing based on the inner
    }
    // FastLED.delay(SPEED);
  }
};


#ifdef USERMOD_WLEDAUDIO

class GEQEffect:public Effect {
public:
  byte previousBarHeight[1024];

  const char * name() {
    return "GEQ";
  }

  void setup() {
    fadeToBlackBy( ledsP, LedsV::nrOfLedsP, 16);
    for (int i=0; i<LedsV::widthV; i++) previousBarHeight[i] = 0;
  }

  void loop() {

    const int NUM_BANDS = NUM_GEQ_CHANNELS ; // map(SEGMENT.custom1, 0, 255, 1, 16);
    const uint16_t cols = LedsV::widthV;
    const uint16_t rows = LedsV::heightV; 


    uint8_t *fftResult = wledAudioMod->fftResults;
    #ifdef SR_DEBUG
    uint8_t samplePeak = *(uint8_t*)um_data->u_data[3];
    #endif

    uint8_t speed = mdl->getValue("speed");
    uint8_t intensity = mdl->getValue("intensity"); 
    bool check1 = mdl->getValue("check1");
    bool check2 = mdl->getValue("check2");

    bool rippleTime = false;
    if (millis() - step >= (256U - intensity)) {
      step = millis();
      rippleTime = true;
    }

    int fadeoutDelay = (256 - speed) / 64;
    if ((fadeoutDelay <= 1 ) || ((call % fadeoutDelay) == 0)) fadeToBlackBy( ledsP, LedsV::nrOfLedsP, speed);

    uint16_t lastBandHeight = 0;  // WLEDMM: for smoothing out bars

    //WLEDMM: evenly ditribut bands
    float bandwidth = (float)cols / NUM_BANDS;
    float remaining = bandwidth;
    uint8_t band = 0;
    for (int x=0; x < cols; x++) {
      //WLEDMM if not enough remaining
      if (remaining < 1) {band++; remaining+= bandwidth;} //increase remaining but keep the current remaining
      remaining--; //consume remaining

      // Serial.printf("x %d b %d n %d w %f %f\n", x, band, NUM_BANDS, bandwidth, remaining);
      uint8_t frBand = ((NUM_BANDS < 16) && (NUM_BANDS > 1)) ? map(band, 0, NUM_BANDS - 1, 0, 15):band; // always use full range. comment out this line to get the previous behaviour.
      // frBand = constrain(frBand, 0, 15); //WLEDMM can never be out of bounds (I think...)
      uint16_t colorIndex = frBand * 17; //WLEDMM 0.255
      uint16_t bandHeight = fftResult[frBand];  // WLEDMM we use the original ffResult, to preserve accuracy

      // WLEDMM begin - smooth out bars
      if ((x > 0) && (x < (cols-1)) && (check2)) {
        // get height of next (right side) bar
        uint8_t nextband = (remaining < 1)? band +1: band;
        nextband = constrain(nextband, 0, 15);  // just to be sure
        frBand = ((NUM_BANDS < 16) && (NUM_BANDS > 1)) ? map(nextband, 0, NUM_BANDS - 1, 0, 15):nextband; // always use full range. comment out this line to get the previous behaviour.
        uint16_t nextBandHeight = fftResult[frBand];
        // smooth Band height
        bandHeight = (7*bandHeight + 3*lastBandHeight + 3*nextBandHeight) / 12;   // yeees, its 12 not 13 (10% amplification)
        bandHeight = constrain(bandHeight, 0, 255);   // remove potential over/underflows
        colorIndex = map(x, 0, cols-1, 0, 255); //WLEDMM
      }
      lastBandHeight = bandHeight; // remember BandHeight (left side) for next iteration
      uint16_t barHeight = map(bandHeight, 0, 255, 0, rows); // Now we map bandHeight to barHeight. do not subtract -1 from rows here
      // WLEDMM end

      if (barHeight > rows) barHeight = rows;                      // WLEDMM map() can "overshoot" due to rounding errors
      if (barHeight > previousBarHeight[x]) previousBarHeight[x] = barHeight; //drive the peak up

      CRGB ledColor = CRGB::Black;
      for (int y=0; y < barHeight; y++) {
        if (check1) //color_vertical / color bars toggle
          colorIndex = map(y, 0, rows-1, 0, 255);

        CRGBPalette16 palette = PartyColors_p;
        ledColor = ColorFromPalette(palette, (uint8_t)colorIndex);

        ledsV.setPixelColor(x + LedsV::widthV * (rows-1 - y), ledColor);
      }

      if ((intensity < 255) && (previousBarHeight[x] > 0) && (previousBarHeight[x] < rows))  // WLEDMM avoid "overshooting" into other segments
        ledsV.setPixelColor(x + LedsV::widthV * (rows - previousBarHeight[x]), ledColor); 

      if (rippleTime && previousBarHeight[x]>0) previousBarHeight[x]--;    //delay/ripple effect

    }
  }

  bool parameters(JsonObject parentVar) {
    ui->initNumber(parentVar, "speed", 255, false);
    ui->initNumber(parentVar, "intensity", 255, false);
    ui->initCheckBox(parentVar, "check1", false, false);
    ui->initCheckBox(parentVar, "check2", false, false);

    // Nice an effect can register it's own DMX channel, but not a fan of repeating the range and type of the param

    e131mod->patchChannel(3, "speed", 255); // TODO: add constant for name
    e131mod->patchChannel(4, "intensity", 255);

    return true;
  }
};

class AudioRings:public RingEffect {
  private:
    uint8_t *fftResult = wledAudioMod->fftResults;

  public:
    const char * name() {
      return "AudioRings";
    }
    void setup() {}

    void setRingFromFtt(int index, int ring) {
      uint8_t val = fftResult[index];
      // Visualize leds to the beat
      CRGB color = ColorFromPalette(palette, val, 255);
      color.nscale8_video(val);
      setRing(ring, color);
    }


    void loop() {
      for (int i = 0; i < 7; i++) {

        uint8_t val;
        if(INWARD) {
          val = fftResult[(i*2)];
        }
        else {
          int b = 14 -(i*2);
          val = fftResult[b];
        }
    
        // Visualize leds to the beat
        CRGB color = ColorFromPalette(palette, val, val);
  //      CRGB color = ColorFromPalette(currentPalette, val, 255, currentBlending);
  //      color.nscale8_video(val);
        setRing(i, color);
  //        setRingFromFtt((i * 2), i); 
      }

      setRingFromFtt(2, 7); // set outer ring to bass
      setRingFromFtt(0, 8); // set outer ring to bass

    }
};


#endif // End Audio Effects

static std::vector<Effect *> effects;