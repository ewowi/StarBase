/*
   @title     StarMod
   @file      AppEffects.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#ifdef STARMOD_USERMOD_WLEDAUDIO
  #include "../User/UserModWLEDAudio.h"
#endif
#ifdef STARMOD_USERMOD_E131
  #include "../User/UserModE131.h"
#endif

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
unsigned long call = 0; //not used at the moment (don't use in effect calculations)
unsigned long now = millis();

//should not contain variables/bytes to keep mem as small as possible!!
class Effect {
public:
  virtual const char * name() {return nullptr;}

  virtual void setup(Leds &leds) {}

  virtual void loop(Leds &leds) {}

  virtual void controls(JsonObject parentVar, Leds &leds) {}

  void addPalette(JsonObject parentVar, uint8_t value) {
    //currentVar["value"][parentRowNr] will be set to value parameter
    JsonObject currentVar = ui->initSelect(parentVar, "pal", value, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Palette");
        ui->setComment(var, "Colors");
        JsonArray options = ui->setOptions(var);
        options.add("CloudColors");
        options.add("LavaColors");
        options.add("OceanColors");
        options.add("ForestColors");
        options.add("RainbowColors");
        options.add("RainbowStripeColors");
        options.add("PartyColors");
        options.add("HeatColors");
        return true;
      }
      default: return false;
    }});
    //tbd: check if memory is freed!
    currentVar["stage"] = true;
  }

  CRGBPalette16 getPalette(uint8_t rowNr) {
    switch (mdl->getValue("pal", rowNr).as<uint8_t>()) {
      case 0: return CloudColors_p; break;
      case 1: return LavaColors_p; break;
      case 2: return OceanColors_p; break;
      case 3: return ForestColors_p; break;
      case 4: return RainbowColors_p; break;
      case 5: return RainbowStripeColors_p; break;
      case 6: return PartyColors_p; break;
      case 7: return HeatColors_p; break;
      default: return PartyColors_p; break;
    }
  }
};

class SolidEffect: public Effect {
public:
  const char * name() {
    return "Solid 1D";
  }
  void loop(Leds &leds) {
    uint8_t red = mdl->getValue("Red", leds.rowNr);
    uint8_t green = mdl->getValue("Green", leds.rowNr);
    uint8_t blue = mdl->getValue("Blue", leds.rowNr);
    CRGB color = CRGB(red, green, blue);
    leds.fill_solid(color);
  }
  void controls(JsonObject parentVar, Leds &leds) {
    ui->initSlider(parentVar, "Red", 182);
    ui->initSlider(parentVar, "Green", 15);
    ui->initSlider(parentVar, "Blue", 98);
  }
};

class RainbowEffect: public Effect {
public:
  const char * name() {
    return "Rainbow 1D";
  }
  void loop(Leds &leds) {
    // FastLED's built-in rainbow generator
    leds.fill_rainbow(gHue, 7);
  }
};

class RainbowWithGlitterEffect:public RainbowEffect {
public:
  const char * name() {
    return "Rainbow with glitter 1D";
  }
  void loop(Leds &leds) {
    // built-in FastLED rainbow, plus some random sparkly glitter
    RainbowEffect::loop(leds);
    addGlitter(leds, 80);
  }
  void addGlitter(Leds &leds, fract8 chanceOfGlitter) 
  {
    if( random8() < chanceOfGlitter) {
      leds[ random16(leds.nrOfLeds) ] += CRGB::White;
    }
  }
};

class SinelonEffect: public Effect {
public:
  const char * name() {
    return "Sinelon 1D";
  }
  void loop(Leds &leds) {
    // a colored dot sweeping back and forth, with fading trails
    leds.fadeToBlackBy(20);
    int pos = beatsin16( mdl->getValue("BPM", leds.rowNr).as<int>(), 0, leds.nrOfLeds-1 );
    leds[pos] = leds.getPixelColor(pos) + CHSV( gHue, 255, 192);
  }
  void controls(JsonObject parentVar, Leds &leds) {
    ui->initSlider(parentVar, "BPM", 60);
  }
}; //Sinelon

//https://www.perfectcircuit.com/signal/difference-between-waveforms
class RunningEffect: public Effect {
public:
  const char * name() {
    return "Running 1D";
  }
  void loop(Leds &leds) {
    // a colored dot sweeping back and forth, with fading trails
    leds.fadeToBlackBy(mdl->getValue("fade", leds.rowNr).as<int>()); //physical leds
    int pos = map(beat16( mdl->getValue("BPM", leds.rowNr).as<int>()), 0, uint16_t(-1), 0, leds.nrOfLeds-1 ); //instead of call%leds.nrOfLeds
    // int pos2 = map(beat16( mdl->getValue("BPM", leds.rowNr).as<int>(), 1000), 0, uint16_t(-1), 0, leds.nrOfLeds-1 ); //one second later
    leds[pos] = CHSV( gHue, 255, 192); //make sure the right physical leds get their value
    // leds[leds.nrOfLeds -1 - pos2] = CHSV( gHue, 255, 192); //make sure the right physical leds get their value
  }
  void controls(JsonObject parentVar, Leds &leds) {
    ui->initSlider(parentVar, "BPM", 60, 0, 255, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setComment(var, "in BPM!");
        return true;
      default: return false;
    }});
    //tbd: check if memory is freed!
    ui->initSlider(parentVar, "fade", 128);
  }
};

class ConfettiEffect: public Effect {
public:
  const char * name() {
    return "Confetti 1D";
  }
  void loop(Leds &leds) {
    // random colored speckles that blink in and fade smoothly
    leds.fadeToBlackBy(10);
    int pos = random16(leds.nrOfLeds);
    leds[pos] += CHSV( gHue + random8(64), 200, 255);
  }
};

class BPMEffect: public Effect {
public:
  const char * name() {
    return "Beats per minute 1D";
  }

  void loop(Leds &leds) {
    CRGBPalette16 pal = getPalette(leds.rowNr);

    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for (uint16_t i = 0; i < leds.nrOfLeds; i++) { //9948
      leds[i] = ColorFromPalette(pal, gHue+(i*2), beat-gHue+(i*10));
    }
  }
  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
  }
};

class JuggleEffect: public Effect {
public:
  const char * name() {
    return "Juggle 1D";
  }
  void loop(Leds &leds) {
    // eight colored dots, weaving in and out of sync with each other
    leds.fadeToBlackBy(20);
    uint8_t dothue = 0;
    for (uint8_t i = 0; i < 8; i++) {
      leds[beatsin16( i+7, 0, leds.nrOfLeds-1 )] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
};

class Ripples3DEffect: public Effect {
public:
  const char * name() {
    return "Ripples 3D";
  }
  void loop(Leds &leds) {
    uint8_t interval = mdl->getValue("interval", leds.rowNr);

    float ripple_interval = 1.3 * (interval/128.0);

    leds.fill_solid(CRGB::Black);
    // fill(CRGB::Black);

    Coord3D pos = {0,0,0};
    for (pos.z=0; pos.z<leds.size.z; pos.z++) {
        for (pos.x=0; pos.x<leds.size.x; pos.x++) {
            float d = leds.fixture->distance(3.5, 3.5, 0, pos.x, pos.z, 0)/9.899495*leds.size.y;
            pos.y = floor(leds.size.y/2.0+sinf(d/ripple_interval + now/100/((256.0-128.0)/20.0))*leds.size.y/2.0); //between 0 and 8

            leds[pos] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
        }
    }
  }
  void controls(JsonObject parentVar, Leds &leds) {
    ui->initSlider(parentVar, "interval", 128);
  }
};

class SphereMove3DEffect: public Effect {
public:
  const char * name() {
    return "SphereMove 3D";
  }
  void loop(Leds &leds) {
    uint16_t origin_x, origin_y, origin_z, d;
    float diameter;

    leds.fill_solid(CRGB::Black);
    // fill(CRGB::Black);

    uint32_t interval = now/100/((256.0-128.0)/20.0);

    Coord3D origin;
    origin.x = 3.5+sinf(interval)*2.5;
    origin.y = 3.5+cosf(interval)*2.5;
    origin.z = 3.5+cosf(interval)*2.0;

    diameter = 2.0+sinf(interval/3.0);

    // CRGBPalette256 pal;
    Coord3D pos;
    for (pos.x=0; pos.x<leds.size.x; pos.x++) {
        for (pos.y=0; pos.y<leds.size.y; pos.y++) {
            for (pos.z=0; pos.z<leds.size.z; pos.z++) {
                d = leds.fixture->distance(pos.x, pos.y, pos.z, origin.x, origin.y, origin.z);

                if (d>diameter && d<diameter+1) {
                  leds[pos] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
                }
            }
        }
    }
  }
}; // SphereMove3DEffect

//Frizzles2D inspired by WLED, Stepko, Andrew Tuline, https://editor.soulmatelights.com/gallery/640-color-frizzles
class Frizzles2D: public Effect {
public:
  const char * name() {
    return "Frizzles 2D";
  }

  void loop(Leds &leds) {
    leds.fadeToBlackBy(16);

    uint16_t bpm = mdl->getValue("BPM", leds.rowNr);
    uint16_t intensity = mdl->getValue("intensity", leds.rowNr);
    CRGBPalette16 pal = getPalette(leds.rowNr);

    for (size_t i = 8; i > 0; i--) {
      Coord3D pos = {0,0,0};
      pos.x = beatsin8(bpm/8 + i, 0, leds.size.x - 1);
      pos.y = beatsin8(intensity/8 - i, 0, leds.size.y - 1);
      CRGB color = ColorFromPalette(pal, beatsin8(12, 0, 255), 255);
      leds[pos] = color;
    }
    leds.blur2d(mdl->getValue("blur", leds.rowNr));
  }

  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
    ui->initSlider(parentVar, "BPM", 60);
    ui->initSlider(parentVar, "intensity", 128);
    ui->initSlider(parentVar, "blur", 128);
  }
}; // Frizzles2D

class Lines2D: public Effect {
public:
  const char * name() {
    return "Lines 2D";
  }

  void loop(Leds &leds) {
    leds.fadeToBlackBy(100);

    Coord3D pos = {0,0,0};
    if (mdl->getValue("Vertical", leds.rowNr).as<bool>()) {
      pos.x = map(beat16( mdl->getValue("BPM", leds.rowNr).as<int>()), 0, uint16_t(-1), 0, leds.size.x-1 ); //instead of call%width

      for (pos.y = 0; pos.y <  leds.size.y; pos.y++) {
        leds[pos] = CHSV( gHue, 255, 192);
      }
    } else {
      pos.y = map(beat16( mdl->getValue("BPM", leds.rowNr).as<int>()), 0, uint16_t(-1), 0, leds.size.y-1 ); //instead of call%height
      for (pos.x = 0; pos.x <  leds.size.x; pos.x++) {
        leds[pos] = CHSV( gHue, 255, 192);
      }
    }
  }

  void controls(JsonObject parentVar, Leds &leds) {
    ui->initSlider(parentVar, "BPM", 60);
    ui->initCheckBox(parentVar, "Vertical", true);
  }
}; // Lines2D

uint8_t gamma8(uint8_t b) { //we do nothing with gamma for now
  return b;
}

//DistortionWaves2D inspired by WLED, ldirko and blazoncek, https://editor.soulmatelights.com/gallery/1089-distorsion-waves
class DistortionWaves2D: public Effect {
public:
  const char * name() {
    return "DistortionWaves 2D";
  }

  void loop(Leds &leds) {

    uint8_t speed = mdl->getValue("speed", leds.rowNr).as<uint8_t>()/32;
    uint8_t scale = mdl->getValue("scale", leds.rowNr).as<uint8_t>()/32;

    uint8_t  w = 2;

    uint16_t a  = now/32;
    uint16_t a2 = a/2;
    uint16_t a3 = a/3;

    uint16_t cx =  beatsin8(10-speed,0,leds.size.x-1)*scale;
    uint16_t cy =  beatsin8(12-speed,0,leds.size.y-1)*scale;
    uint16_t cx1 = beatsin8(13-speed,0,leds.size.x-1)*scale;
    uint16_t cy1 = beatsin8(15-speed,0,leds.size.y-1)*scale;
    uint16_t cx2 = beatsin8(17-speed,0,leds.size.x-1)*scale;
    uint16_t cy2 = beatsin8(14-speed,0,leds.size.y-1)*scale;
    
    uint16_t xoffs = 0;
    Coord3D pos = {0,0,0};
    for (pos.x = 0; pos.x < leds.size.x; pos.x++) {
      xoffs += scale;
      uint16_t yoffs = 0;

      for (pos.y = 0; pos.y < leds.size.y; pos.y++) {
        yoffs += scale;

        byte rdistort = cos8((cos8(((pos.x<<3)+a )&255)+cos8(((pos.y<<3)-a2)&255)+a3   )&255)>>1; 
        byte gdistort = cos8((cos8(((pos.x<<3)-a2)&255)+cos8(((pos.y<<3)+a3)&255)+a+32 )&255)>>1; 
        byte bdistort = cos8((cos8(((pos.x<<3)+a3)&255)+cos8(((pos.y<<3)-a) &255)+a2+64)&255)>>1; 

        byte valueR = rdistort+ w*  (a- ( ((xoffs - cx)  * (xoffs - cx)  + (yoffs - cy)  * (yoffs - cy))>>7  ));
        byte valueG = gdistort+ w*  (a2-( ((xoffs - cx1) * (xoffs - cx1) + (yoffs - cy1) * (yoffs - cy1))>>7 ));
        byte valueB = bdistort+ w*  (a3-( ((xoffs - cx2) * (xoffs - cx2) + (yoffs - cy2) * (yoffs - cy2))>>7 ));

        valueR = gamma8(cos8(valueR));
        valueG = gamma8(cos8(valueG));
        valueB = gamma8(cos8(valueB));

        leds[pos] = CRGB(valueR, valueG, valueB);
      }
    }
  }
  void controls(JsonObject parentVar, Leds &leds) {
    ui->initSlider(parentVar, "speed", 128);
    ui->initSlider(parentVar, "scale", 128);
  }
}; // DistortionWaves2D

//Octopus2D inspired by WLED, Stepko and Sutaburosu and blazoncek 
//Idea from https://www.youtube.com/watch?v=HsA-6KIbgto&ab_channel=GreatScott%21 (https://editor.soulmatelights.com/gallery/671-octopus)
class Octopus2D: public Effect {
public:
  const char * name() {
    return "Octopus 2D";
  }

  typedef struct {
    uint8_t angle;
    uint8_t radius;
  } map_t;

  void loop(Leds &leds) {

    const uint8_t mapp = 180 / max(leds.size.x,leds.size.y);

    uint8_t speed = mdl->getValue("speed", leds.rowNr);
    uint8_t offsetX = mdl->getValue("Offset X", leds.rowNr);
    uint8_t offsetY = mdl->getValue("Offset Y", leds.rowNr);
    uint8_t legs = mdl->getValue("Legs", leds.rowNr);
    CRGBPalette16 pal = getPalette(leds.rowNr);

    leds.sharedData.allocate(sizeof(map_t) * leds.size.x * leds.size.y + 2 * sizeof(uint8_t) + 2 * sizeof(uint16_t) + sizeof(uint32_t));
    map_t *rMap = leds.sharedData.bind<map_t>(leds.size.x * leds.size.y); //array
    uint8_t *offsX = leds.sharedData.bind<uint8_t>();
    uint8_t *offsY = leds.sharedData.bind<uint8_t>();
    uint16_t *aux0 = leds.sharedData.bind<uint16_t>();
    uint16_t *aux1 = leds.sharedData.bind<uint16_t>();
    uint32_t *step = leds.sharedData.bind<uint32_t>();
    if (!leds.sharedData.allocated()) return;

    Coord3D pos = {0,0,0};

    // re-init if SEGMENT dimensions or offset changed
    if (*aux0 != leds.size.x || *aux1 != leds.size.y || offsetX != *offsX || offsetY != *offsY) {
      // *step = 0;
      *aux0 = leds.size.x;
      *aux1 = leds.size.y;
      *offsX = offsetX;
      *offsY = offsetY;
      const uint8_t C_X = leds.size.x / 2 + (offsetX - 128)*leds.size.x/255;
      const uint8_t C_Y = leds.size.y / 2 + (offsetY - 128)*leds.size.y/255;
      for (pos.x = 0; pos.x < leds.size.x; pos.x++) {
        for (pos.y = 0; pos.y < leds.size.y; pos.y++) {
          rMap[leds.XY(pos.x, pos.y)].angle = 40.7436f * atan2f(pos.y - C_Y, pos.x - C_X); // avoid 128*atan2()/PI
          rMap[leds.XY(pos.x, pos.y)].radius = hypotf(pos.x - C_X, pos.y - C_Y) * mapp; //thanks Sutaburosu
        }
      }
    }

    *step = now * speed / 32 / 10;//mdl->getValue("realFps", leds.rowNr).as<int>();  // WLEDMM 40fps

    for (pos.x = 0; pos.x < leds.size.x; pos.x++) {
      for (pos.y = 0; pos.y < leds.size.y; pos.y++) {
        byte angle = rMap[leds.XY(pos.x,pos.y)].angle;
        byte radius = rMap[leds.XY(pos.x,pos.y)].radius;
        //CRGB c = CHSV(SEGENV.step / 2 - radius, 255, sin8(sin8((angle * 4 - radius) / 4 + SEGENV.step) + radius - SEGENV.step * 2 + angle * (SEGMENT.custom3/3+1)));
        uint16_t intensity = sin8(sin8((angle * 4 - radius) / 4 + *step/2) + radius - *step + angle * legs);
        intensity = map(intensity*intensity, 0, UINT16_MAX, 0, 255); // add a bit of non-linearity for cleaner display
        CRGB color = ColorFromPalette(pal, *step / 2 - radius, intensity);
        leds[pos] = color;
      }
    }
  }
  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
    ui->initSlider(parentVar, "speed", 128, 1, 255); //start with speed 1
    ui->initSlider(parentVar, "Offset X", 128);
    ui->initSlider(parentVar, "Offset Y", 128);
    ui->initSlider(parentVar, "Legs", 4, 1, 8);
  }
}; // Octopus2D

//Lissajous2D inspired by WLED, Andrew Tuline 
class Lissajous2D: public Effect {
public:
  const char * name() {
    return "Lissajous 2D";
  }

  void loop(Leds &leds) {

    uint8_t freqX = mdl->getValue("X frequency", leds.rowNr);
    uint8_t fadeRate = mdl->getValue("Fade rate", leds.rowNr);
    uint8_t speed = mdl->getValue("speed", leds.rowNr);
    bool smooth = mdl->getValue("Smooth", leds.rowNr);
    CRGBPalette16 pal = getPalette(leds.rowNr);

    leds.fadeToBlackBy(fadeRate);

    uint_fast16_t phase = now * speed / 256;  // allow user to control rotation speed, speed between 0 and 255!

    Coord3D locn = {0,0,0};
    if (smooth) { // WLEDMM: this is the original "float" code featuring anti-aliasing
        int maxLoops = max(192, 4*(leds.size.x+leds.size.y));
        maxLoops = ((maxLoops / 128) +1) * 128; // make sure whe have half or full turns => multiples of 128
        for (int i=0; i < maxLoops; i ++) {
          locn.x = float(sin8(phase/2 + (i* freqX)/64)) / 255.0f;  // WLEDMM align speed with original effect
          locn.y = float(cos8(phase/2 + i*2)) / 255.0f;
          //SEGMENT.setPixelColorXY(xlocn, ylocn, SEGMENT.color_from_palette(strip.now/100+i, false, PALETTE_SOLID_WRAP, 0)); // draw pixel with anti-aliasing
          unsigned palIndex = (256*locn.y) + phase/2 + (i* freqX)/64;
          // SEGMENT.setPixelColorXY(xlocn, ylocn, SEGMENT.color_from_palette(palIndex, false, PALETTE_SOLID_WRAP, 0)); // draw pixel with anti-aliasing - color follows rotation
          leds[locn] = ColorFromPalette(pal, palIndex);
        }
    } else
    for (int i=0; i < 256; i ++) {
      //WLEDMM: stick to the original calculations of xlocn and ylocn
      locn.x = sin8(phase/2 + (i*freqX)/64);
      locn.y = cos8(phase/2 + i*2);
      locn.x = (leds.size.x < 2) ? 1 : (map(2*locn.x, 0,511, 0,2*(leds.size.x-1)) +1) /2;    // softhack007: "*2 +1" for proper rounding
      locn.y = (leds.size.y < 2) ? 1 : (map(2*locn.y, 0,511, 0,2*(leds.size.y-1)) +1) /2;    // "leds.size.y > 2" is needed to avoid div/0 in map()
      // SEGMENT.setPixelColorXY((uint8_t)xlocn, (uint8_t)ylocn, SEGMENT.color_from_palette(strip.now/100+i, false, PALETTE_SOLID_WRAP, 0));
      leds[locn] = ColorFromPalette(pal, now/100+i);
    }
  }
  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
    ui->initSlider(parentVar, "X frequency", 64);
    ui->initSlider(parentVar, "Fade rate", 128);
    ui->initSlider(parentVar, "speed", 128);
    ui->initCheckBox(parentVar, "Smooth", false);
  }
}; // Lissajous2D


#define maxNumBalls 16

//BouncingBalls1D  inspired by WLED
//each needs 12 bytes
typedef struct Ball {
  unsigned long lastBounceTime;
  float impactVelocity;
  float height;
} ball;

class BouncingBalls1D: public Effect {
public:
  const char * name() {
    return "Bouncing Balls 1D";
  }

  void loop(Leds &leds) {
    uint8_t grav = mdl->getValue("gravity", leds.rowNr);
    uint8_t numBalls = mdl->getValue("balls", leds.rowNr);
    CRGBPalette16 pal = getPalette(leds.rowNr);

    leds.sharedData.allocate(sizeof(Ball) * maxNumBalls);
    Ball *balls = leds.sharedData.bind<Ball>(maxNumBalls); //array
    if (!leds.sharedData.allocated()) return;

    leds.fill_solid(CRGB::Black);

    // non-chosen color is a random color
    const float gravity = -9.81f; // standard value of gravity
    // const bool hasCol2 = SEGCOLOR(2);
    const unsigned long time = now;

    //not necessary as sharedData is cleared at setup(Leds &leds)
    // if (call == 0) {
    //   for (size_t i = 0; i < maxNumBalls; i++) balls[i].lastBounceTime = time;
    // }

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

      int pos = roundf(balls[i].height * (leds.nrOfLeds - 1));

      CRGB color = ColorFromPalette(pal, i*(256/max(numBalls, (uint8_t)8)), 255);

      leds[pos] = color;
      // if (SEGLEN<32) SEGMENT.setPixelColor(indexToVStrip(pos, stripNr), color); // encode virtual strip into index
      // else           SEGMENT.setPixelColor(balls[i].height + (stripNr+1)*10.0f, color);
    } //balls
  }

  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
    ui->initSlider(parentVar, "gravity", 128);
    ui->initSlider(parentVar, "balls", 8, 1, 16);
  }
}; // BouncingBalls2D

class RingEffect:public Effect {
  protected:

    void setRing(Leds &leds, int ring, CRGB colour) { //so britisch ;-)
      leds[ring] = colour;
    }

};

class RingRandomFlow:public RingEffect {
public:
  const char * name() {
    return "RingRandomFlow 1D";
  }

  void loop(Leds &leds) {
    leds.sharedData.allocate(sizeof(uint8_t) * leds.nrOfLeds);
    uint8_t *hue = leds.sharedData.bind<uint8_t>(leds.nrOfLeds); //array
    if (!leds.sharedData.allocated()) return;

    hue[0] = random(0, 255);
    for (int r = 0; r < leds.nrOfLeds; r++) {
      setRing(leds, r, CHSV(hue[r], 255, 255));
    }
    for (int r = (leds.nrOfLeds - 1); r >= 1; r--) {
      hue[r] = hue[(r - 1)]; // set this ruing based on the inner
    }
    // FastLED.delay(SPEED);
  }
};

class ScrollingText2D: public Effect {
public:
  const char * name() {
    return "Scrolling Text 2D";
  }

  void loop(Leds &leds) {
    uint8_t speed = mdl->getValue("speed", leds.rowNr);
    uint8_t font = mdl->getValue("font", leds.rowNr);
    const char * text = mdl->getValue("text", leds.rowNr);

    // text might be nullified by selecting other effects and if effect is selected, controls are run afterwards  
    // tbd: this should be removed and setEffect must make sure this cannot happen!!
    if (text && strlen(text)>0) {
      leds.fadeToBlackBy();
      leds.drawText(text, 0, 0, font, CRGB::Red, - (call*speed/256));
    }

  }
  void controls(JsonObject parentVar, Leds &leds) {
    ui->initText(parentVar, "text", "StarMod");
    ui->initSlider(parentVar, "speed", 128);
    ui->initSelect(parentVar, "font", 0, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun: {
        JsonArray options = ui->setOptions(var);
        options.add("4x6");
        options.add("5x8");
        options.add("5x12");
        options.add("6x8");
        options.add("7x9");
        return true;
      }
      default: return false;
    }});
  }
};


class Waverly2D: public Effect {
public:
  const char * name() {
    return "Waverly 2D";
  }

  void loop(Leds &leds) {
    CRGBPalette16 pal = getPalette(leds.rowNr);
    uint8_t amplification = mdl->getValue("Amplification", leds.rowNr);
    uint8_t sensitivity = mdl->getValue("Sensitivity", leds.rowNr);
    bool noClouds = mdl->getValue("No Clouds", leds.rowNr);
    // bool soundPressure = mdl->getValue("Sound Pressure", leds.rowNr);
    // bool agcDebug = mdl->getValue("AGC debug", leds.rowNr);

    leds.fadeToBlackBy(amplification);
    // if (agcDebug && soundPressure) soundPressure = false;                 // only one of the two at any time
    // if ((soundPressure) && (wledAudioMod->sync.volumeSmth > 0.5f)) wledAudioMod->sync.volumeSmth = wledAudioMod->sync.soundPressure;    // show sound pressure instead of volume
    // if (agcDebug) wledAudioMod->sync.volumeSmth = 255.0 - wledAudioMod->sync.agcSensitivity;                    // show AGC level instead of volume

    long t = now / 2; 
    Coord3D pos;
    for (pos.x = 0; pos.x < leds.size.x; pos.x++) {
      uint16_t thisVal = wledAudioMod->sync.volumeSmth*sensitivity/64 * inoise8(pos.x * 45 , t , t)/64;      // WLEDMM back to SR code
      uint16_t thisMax = min(map(thisVal, 0, 512, 0, leds.size.y), (long)leds.size.x);

      for (pos.y = 0; pos.y < thisMax; pos.y++) {
        if (!noClouds)
          leds.addPixelColor(pos, ColorFromPalette(pal, map(pos.y, 0, thisMax, 250, 0), 255, LINEARBLEND));
        leds.addPixelColor(leds.XY((leds.size.x - 1) - pos.x, (leds.size.y - 1) - pos.y), ColorFromPalette(pal, map(pos.y, 0, thisMax, 250, 0), 255, LINEARBLEND));
      }
    }
    leds.blur2d(16);

  }
  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
    ui->initSlider(parentVar, "Amplification", 128);
    ui->initSlider(parentVar, "Sensitivity", 128);
    ui->initCheckBox(parentVar, "No Clouds");
    // ui->initCheckBox(parentVar, "Sound Pressure");
    // ui->initCheckBox(parentVar, "AGC debug");
  }
};

#define maxNumPopcorn 21 // max 21 on 16 segment ESP8266
#define NUM_COLORS       3 /* number of colors per segment */

//each needs 19 bytes
//Spark type is used for popcorn, 1D fireworks, and drip
typedef struct Spark {
  float pos, posX;
  float vel, velX;
  uint16_t col;
  uint8_t colIndex;
} spark;


class PopCorn1D: public Effect {
public:
  const char * name() {
    return "PopCorn 1D";
  }

  void loop(Leds &leds) {
    CRGBPalette16 pal = getPalette(leds.rowNr);
    uint8_t speed = mdl->getValue("speed", leds.rowNr);
    uint8_t intensity = mdl->getValue("intensity", leds.rowNr);
    bool useaudio = mdl->getValue("useaudio", leds.rowNr);

    leds.sharedData.allocate(sizeof(Spark) * maxNumPopcorn);
    Spark *popcorn = leds.sharedData.bind<Spark>(maxNumPopcorn); //array
    if (!leds.sharedData.allocated()) return;

    float gravity = -0.0001 - (speed/200000.0); // m/s/s
    gravity *= leds.nrOfLeds;

    uint8_t numPopcorn = intensity*maxNumPopcorn/255;
    if (numPopcorn == 0) numPopcorn = 1;

    for(int i = 0; i < numPopcorn; i++) {
      if (popcorn[i].pos >= 0.0f) { // if kernel is active, update its position
        popcorn[i].pos += popcorn[i].vel;
        popcorn[i].vel += gravity;
      } else { // if kernel is inactive, randomly pop it
        bool doPopCorn = false;  // WLEDMM allows to inhibit new pops
        // WLEDMM begin
        if (useaudio) {
          if (  (wledAudioMod->sync.volumeSmth > 1.0f)                      // no pops in silence
              // &&((wledAudioMod->sync.samplePeak > 0) || (wledAudioMod->sync.volumeRaw > 128))  // try to pop at onsets (our peek detector still sucks)
              &&(random8() < 4) )                        // stay somewhat random
            doPopCorn = true;
        } else {         
          if (random8() < 2) doPopCorn = true; // default POP!!!
        }
        // WLEDMM end

        if (doPopCorn) { // POP!!!
          popcorn[i].pos = 0.01f;

          uint16_t peakHeight = 128 + random8(128); //0-255
          peakHeight = (peakHeight * (leds.nrOfLeds -1)) >> 8;
          popcorn[i].vel = sqrtf(-2.0f * gravity * peakHeight);

          // if (SEGMENT.palette)
          // {
            popcorn[i].colIndex = random8();
          // } else {
          //   byte col = random8(0, NUM_COLORS);
          //   if (!SEGCOLOR(2) || !SEGCOLOR(col)) col = 0;
          //   popcorn[i].colIndex = col;
          // }
        }
      }
      if (popcorn[i].pos >= 0.0f) { // draw now active popcorn (either active before or just popped)
        // uint32_t col = SEGMENT.color_wheel(popcorn[i].colIndex);
        // if (!SEGMENT.palette && popcorn[i].colIndex < NUM_COLORS) col = SEGCOLOR(popcorn[i].colIndex);
        uint16_t ledIndex = popcorn[i].pos;
        CRGB col = ColorFromPalette(pal, popcorn[i].colIndex*(256/maxNumPopcorn), 255);
        if (ledIndex < leds.nrOfLeds) leds.setPixelColor(ledIndex, col);
      }
    }

  }
  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
    ui->initSlider(parentVar, "speed", 128);
    ui->initSlider(parentVar, "intensity", 128);
    ui->initCheckBox(parentVar, "useaudio");
    ui->initSlider(parentVar, "nrOfPopCorn", 10, 1, 21);
  }
}; //PopCorn1D


#ifdef STARMOD_USERMOD_WLEDAUDIO

class GEQEffect:public Effect {
public:
  const char * name() {
    return "GEQ 2D";
  }

  void setup(Leds &leds) {
    leds.fadeToBlackBy(16);
  }

  void loop(Leds &leds) {
    leds.sharedData.allocate(sizeof(uint16_t) * leds.size.x + sizeof(uint32_t));
    uint16_t *previousBarHeight = leds.sharedData.bind<uint16_t>(leds.size.x); //array
    uint32_t *step = leds.sharedData.bind<uint32_t>();
    if (!leds.sharedData.allocated()) return;

    const int NUM_BANDS = NUM_GEQ_CHANNELS ; // map(SEGMENT.custom1, 0, 255, 1, 16);

    #ifdef SR_DEBUG
    uint8_t samplePeak = *(uint8_t*)um_data->u_data[3];
    #endif

    uint8_t fadeOut = mdl->getValue("fadeOut", leds.rowNr);
    uint8_t ripple = mdl->getValue("ripple", leds.rowNr); 
    bool colorBars = mdl->getValue("colorBars", leds.rowNr);
    bool smoothBars = mdl->getValue("smoothBars", leds.rowNr);
    CRGBPalette16 pal = getPalette(leds.rowNr);

    bool rippleTime = false;
    if (now - *step >= (256U - ripple)) {
      *step = now;
      rippleTime = true;
    }

    int fadeoutDelay = (256 - fadeOut) / 64; //256..1 -> 4..0
    size_t beat = map(beat16( fadeOut), 0, uint16_t(-1), 0, fadeoutDelay-1 ); // instead of call%fadeOutDelay

    if ((fadeoutDelay <= 1 ) || (beat == 0)) leds.fadeToBlackBy(fadeOut);

    uint16_t lastBandHeight = 0;  // WLEDMM: for smoothing out bars

    //WLEDMM: evenly ditribut bands
    float bandwidth = (float)leds.size.x / NUM_BANDS;
    float remaining = bandwidth;
    uint8_t band = 0;
    Coord3D pos = {0,0,0};
    for (pos.x=0; pos.x < leds.size.x; pos.x++) {
      //WLEDMM if not enough remaining
      if (remaining < 1) {band++; remaining+= bandwidth;} //increase remaining but keep the current remaining
      remaining--; //consume remaining

      // USER_PRINTF("x %d b %d n %d w %f %f\n", x, band, NUM_BANDS, bandwidth, remaining);
      uint8_t frBand = ((NUM_BANDS < 16) && (NUM_BANDS > 1)) ? map(band, 0, NUM_BANDS - 1, 0, 15):band; // always use full range. comment out this line to get the previous behaviour.
      // frBand = constrain(frBand, 0, 15); //WLEDMM can never be out of bounds (I think...)
      uint16_t colorIndex = frBand * 17; //WLEDMM 0.255
      uint16_t bandHeight = wledAudioMod->fftResults[frBand];  // WLEDMM we use the original ffResult, to preserve accuracy

      // WLEDMM begin - smooth out bars
      if ((pos.x > 0) && (pos.x < (leds.size.x-1)) && (smoothBars)) {
        // get height of next (right side) bar
        uint8_t nextband = (remaining < 1)? band +1: band;
        nextband = constrain(nextband, 0, 15);  // just to be sure
        frBand = ((NUM_BANDS < 16) && (NUM_BANDS > 1)) ? map(nextband, 0, NUM_BANDS - 1, 0, 15):nextband; // always use full range. comment out this line to get the previous behaviour.
        uint16_t nextBandHeight = wledAudioMod->fftResults[frBand];
        // smooth Band height
        bandHeight = (7*bandHeight + 3*lastBandHeight + 3*nextBandHeight) / 12;   // yeees, its 12 not 13 (10% amplification)
        bandHeight = constrain(bandHeight, 0, 255);   // remove potential over/underflows
        colorIndex = map(pos.x, 0, leds.size.x-1, 0, 255); //WLEDMM
      }
      lastBandHeight = bandHeight; // remember BandHeight (left side) for next iteration
      uint16_t barHeight = map(bandHeight, 0, 255, 0, leds.size.y); // Now we map bandHeight to barHeight. do not subtract -1 from leds.size.y here
      // WLEDMM end

      if (barHeight > leds.size.y) barHeight = leds.size.y;                      // WLEDMM map() can "overshoot" due to rounding errors
      if (barHeight > previousBarHeight[pos.x]) previousBarHeight[pos.x] = barHeight; //drive the peak up

      CRGB ledColor = CRGB::Black;

      for (pos.y=0; pos.y < barHeight; pos.y++) {
        if (colorBars) //color_vertical / color bars toggle
          colorIndex = map(pos.y, 0, leds.size.y-1, 0, 255);

        ledColor = ColorFromPalette(pal, (uint8_t)colorIndex);

        leds.setPixelColor(leds.XY(pos.x, leds.size.y - 1 - pos.y), ledColor);
      }

      if ((ripple > 0) && (previousBarHeight[pos.x] > 0) && (previousBarHeight[pos.x] < leds.size.y))  // WLEDMM avoid "overshooting" into other segments
        leds.setPixelColor(leds.XY(pos.x, leds.size.y - previousBarHeight[pos.x]), CHSV( gHue, 255, 192)); // take gHue color for the time being

      if (rippleTime && previousBarHeight[pos.x]>0) previousBarHeight[pos.x]--;    //delay/ripple effect

    }
  }

  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
    ui->initSlider(parentVar, "fadeOut", 255);
    ui->initSlider(parentVar, "ripple", 128);
    ui->initCheckBox(parentVar, "colorBars", false);
    ui->initCheckBox(parentVar, "smoothBars", true);

    // Nice an effect can register it's own DMX channel, but not a fan of repeating the range and type of the param

    // #ifdef STARMOD_USERMOD_E131

    //   if (e131mod->isEnabled) {
    //     e131mod->patchChannel(3, "fadeOut", 255); // TODO: add constant for name
    //     e131mod->patchChannel(4, "ripple", 255);
    //     for (JsonObject childVar: mdl->findVar("e131Tbl")["n"].as<JsonArray>()) {
    //       ui->callVarFun(childVar, UINT8_MAX, f_UIFun);
    //     }
    //   }

    // #endif
  }
};

class AudioRings:public RingEffect {
public:
  const char * name() {
    return "AudioRings 1D";
  }

  void loop(Leds &leds) {
    CRGBPalette16 pal = getPalette(leds.rowNr);
    for (int i = 0; i < 7; i++) { // 7 rings

      uint8_t val;
      if(mdl->getValue("inWards", leds.rowNr).as<bool>()) {
        val = wledAudioMod->fftResults[(i*2)];
      }
      else {
        int b = 14 -(i*2);
        val = wledAudioMod->fftResults[b];
      }
  
      // Visualize leds to the beat
      CRGB color = ColorFromPalette(pal, val, val);
//      CRGB color = ColorFromPalette(currentPalette, val, 255, currentBlending);
//      color.nscale8_video(val);
      setRing(leds, i, color);
//        setRingFromFtt((i * 2), i); 
    }

    setRingFromFtt(leds, pal, 2, 7); // set outer ring to bass
    setRingFromFtt(leds, pal, 0, 8); // set outer ring to bass

  }
  void setRingFromFtt(Leds &leds, CRGBPalette16 pal, int index, int ring) {
    uint8_t val = wledAudioMod->fftResults[index];
    // Visualize leds to the beat
    CRGB color = ColorFromPalette(pal, val, 255);
    color.nscale8_video(val);
    setRing(leds, ring, color);
  }

  void controls(JsonObject parentVar, Leds &leds) {
    addPalette(parentVar, 4);
    ui->initCheckBox(parentVar, "inWards", true);
  }
};

class FreqMatrix:public Effect {
public:
  const char * name() {
    return "FreqMatrix 1D";
  }

  void setup(Leds &leds) {
    leds.fadeToBlackBy(16);
  }

  void loop(Leds &leds) {
    leds.sharedData.allocate(sizeof(uint8_t));
    uint8_t *aux0 = leds.sharedData.bind<uint8_t>();
    if (!leds.sharedData.allocated()) return;

    uint8_t speed = mdl->getValue("speed", leds.rowNr);
    uint8_t fx = mdl->getValue("Sound effect", leds.rowNr);
    uint8_t lowBin = mdl->getValue("Low bin", leds.rowNr);
    uint8_t highBin = mdl->getValue("High bin", leds.rowNr);
    uint8_t sensitivity10 = mdl->getValue("Sensivity", leds.rowNr);

    uint8_t secondHand = (speed < 255) ? (micros()/(256-speed)/500 % 16) : 0;
    if((speed > 254) || (*aux0 != secondHand)) {   // WLEDMM allow run run at full speed
      *aux0 = secondHand;

      // Pixel brightness (value) based on volume * sensitivity * intensity
      // uint_fast8_t sensitivity10 = map(sensitivity, 0, 31, 10, 100); // reduced resolution slider // WLEDMM sensitivity * 10, to avoid losing precision
      int pixVal = wledAudioMod->sync.volumeSmth * (float)fx * (float)sensitivity10 / 2560.0f; // WLEDMM 2560 due to sensitivity * 10
      if (pixVal > 255) pixVal = 255;  // make a brightness from the last avg

      CRGB color = CRGB::Black;

      if (wledAudioMod->sync.FFT_MajorPeak > MAX_FREQUENCY) wledAudioMod->sync.FFT_MajorPeak = 1;
      // MajorPeak holds the freq. value which is most abundant in the last sample.
      // With our sampling rate of 10240Hz we have a usable freq range from roughtly 80Hz to 10240/2 Hz
      // we will treat everything with less than 65Hz as 0

      if ((wledAudioMod->sync.FFT_MajorPeak > 80.0f) && (wledAudioMod->sync.volumeSmth > 0.25f)) { // WLEDMM
        // Pixel color (hue) based on major frequency
        int upperLimit = 80 + 42 * highBin;
        int lowerLimit = 80 + 3 * lowBin;
        //uint8_t i =  lowerLimit!=upperLimit ? map(FFT_MajorPeak, lowerLimit, upperLimit, 0, 255) : FFT_MajorPeak;  // (original formula) may under/overflow - so we enforce uint8_t
        int freqMapped =  lowerLimit!=upperLimit ? map(wledAudioMod->sync.FFT_MajorPeak, lowerLimit, upperLimit, 0, 255) : wledAudioMod->sync.FFT_MajorPeak;  // WLEDMM preserve overflows
        uint8_t i = abs(freqMapped) & 0xFF;  // WLEDMM we embrace overflow ;-) by "modulo 256"

        color = CHSV(i, 240, (uint8_t)pixVal); // implicit conversion to RGB supplied by FastLED
      }

      // shift the pixels one pixel up
      leds.setPixelColor(0, color);
      for (int i = leds.nrOfLeds - 1; i > 0; i--) leds.setPixelColor(i, leds.getPixelColor(i-1));
    }
  }

  void controls(JsonObject parentVar, Leds &leds) {
    ui->initSlider(parentVar, "speed", 255);
    ui->initSlider(parentVar, "Sound effect", 128);
    ui->initSlider(parentVar, "Low bin", 18);
    ui->initSlider(parentVar, "High bin", 48);
    ui->initSlider(parentVar, "Sensivity", 30, 10, 100);
  }
};

#endif // End Audio Effects

class Effects {
public:
  std::vector<Effect *> effects;

  Effects() {
    //create effects before fx.chFun is called
    effects.push_back(new SolidEffect);
    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new RunningEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new JuggleEffect);
    effects.push_back(new Ripples3DEffect);
    effects.push_back(new SphereMove3DEffect);
    effects.push_back(new Frizzles2D);
    effects.push_back(new Lines2D);
    effects.push_back(new DistortionWaves2D);
    effects.push_back(new Octopus2D);
    effects.push_back(new Lissajous2D);
    effects.push_back(new BouncingBalls1D);
    effects.push_back(new RingRandomFlow);
    effects.push_back(new ScrollingText2D);
    effects.push_back(new Waverly2D);
    effects.push_back(new PopCorn1D);
    #ifdef STARMOD_USERMOD_WLEDAUDIO
      effects.push_back(new GEQEffect);
      effects.push_back(new AudioRings);
      effects.push_back(new FreqMatrix);
    #endif
  }

  void setup() {
    //check of no local variables (should be only 4 bytes): tbd: can we loop over effects (sizeof(effect does not work))
    // for (Effect *effect:effects) {
    //     USER_PRINTF("Size of %s is %d\n", effect->name(), sizeof(*effect));
    // }
    // USER_PRINTF("Size of %s is %d\n", "RainbowEffect", sizeof(RainbowEffect));
    // USER_PRINTF("Size of %s is %d\n", "RainbowWithGlitterEffect", sizeof(RainbowWithGlitterEffect));
    // USER_PRINTF("Size of %s is %d\n", "SinelonEffect", sizeof(SinelonEffect));
    // USER_PRINTF("Size of %s is %d\n", "RunningEffect", sizeof(RunningEffect));
    // USER_PRINTF("Size of %s is %d\n", "ConfettiEffect", sizeof(ConfettiEffect));
    // USER_PRINTF("Size of %s is %d\n", "BPMEffect", sizeof(BPMEffect));
    // USER_PRINTF("Size of %s is %d\n", "JuggleEffect", sizeof(JuggleEffect));
    // USER_PRINTF("Size of %s is %d\n", "Ripples3DEffect", sizeof(Ripples3DEffect));
    // USER_PRINTF("Size of %s is %d\n", "SphereMove3DEffect", sizeof(SphereMove3DEffect));
    // USER_PRINTF("Size of %s is %d\n", "Frizzles2D", sizeof(Frizzles2D));
    // USER_PRINTF("Size of %s is %d\n", "Lines2D", sizeof(Lines2D));
    // USER_PRINTF("Size of %s is %d\n", "DistortionWaves2D", sizeof(DistortionWaves2D));
    // USER_PRINTF("Size of %s is %d\n", "Octopus2D", sizeof(Octopus2D));
    // USER_PRINTF("Size of %s is %d\n", "Lissajous2D", sizeof(Lissajous2D));
    // USER_PRINTF("Size of %s is %d\n", "BouncingBalls1D", sizeof(BouncingBalls1D));
    // USER_PRINTF("Size of %s is %d\n", "RingRandomFlow", sizeof(RingRandomFlow));
    // #ifdef STARMOD_USERMOD_WLEDAUDIO
    //   USER_PRINTF("Size of %s is %d\n", "GEQEffect", sizeof(GEQEffect));
    //   USER_PRINTF("Size of %s is %d\n", "AudioRings", sizeof(AudioRings));
    // #endif
  }

  void loop(Leds &leds) {
    now = millis(); //tbd timebase

    effects[leds.fx%effects.size()]->loop(leds);

    #ifdef STARMOD_USERMOD_WLEDAUDIO

      if (mdl->getValue("mHead") ) {
        leds.fixture->head.x = wledAudioMod->fftResults[3];
        leds.fixture->head.y = wledAudioMod->fftResults[8];
        leds.fixture->head.z = wledAudioMod->fftResults[13];
      }

    #endif

    call++;

    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }

  void setEffect(Leds &leds, JsonObject var, uint8_t rowNr) {

    leds.fx = mdl->getValue(var, rowNr);

    USER_PRINTF("setEffect fx[%d]: %d\n", rowNr, leds.fx);

    if (leds.fx < effects.size()) {

      leds.sharedData.clear(); //make sure all values are 0

      for (JsonObject var: var["n"].as<JsonArray>()) { //for all controls
        if (mdl->varOrder(var) >= 0) { //post init
          mdl->varOrder(var, -mdl->varOrder(var)); // set all negative
        }
      }

      Effect* effect = effects[leds.fx];

      ui->parentRowNr = rowNr;
      effect->controls(var, leds); //new controls have positive order (var["o"])
      ui->parentRowNr = UINT8_MAX;

      effect->setup(leds); //if changed then run setup once (like call==0 in WLED)

      //check if post init added
      bool postInit = false;
      for (JsonObject childVar: mdl->varN(var)) {

        if (mdl->varOrder(childVar) >= 0) { //post init, just added, 
          postInit = true;
          break;
        }
      }
      if (postInit) {
        for (JsonObject childVar: mdl->varN(var)) {
          if (childVar["value"].is<JsonArray>())
          {
            JsonArray valArray = childVar["value"];

            if (mdl->varOrder(childVar) < 0) { //if not updated
              valArray[rowNr] = (char*)0; //null
              // mdl->setValue(var, -99, rowNr); //set value -99
              mdl->varOrder(childVar, -mdl->varOrder(childVar)); //make positive again
              //if some values in array are not -99
            }

            //if all values null, remove value
            bool allNull = true;
            for (JsonVariant element: valArray) {
              if (!element.isNull())
                allNull = false;
            }
            if (allNull) {
              print->printJson("remove allnulls", childVar);
              mdl->varN(var).remove(childVar);
            }
          }

        }
      }
      
      print->printJson("control", var);
        // if (mdl->varOrder(var) >= 0) { //post init
        //   var["o"] = -mdl->varOrder(var); //make positive again
        //set unused vars to inactive 
        // if (mdl->varOrder(var) >=0)
        //   mdl->setValue(var, UINT16_MAX, rowNr);
      // }
      // for (JsonObject var: var["n"].as<JsonArray>()) {
      //   if (mdl->varOrder(var) <0)
      //     var["o"] = -mdl->varOrder(var);
      // }
      //remove vars with all values -99

      //tbd: make property of effects
      if (strstr(effects[leds.fx]->name(), "2D")) {
        if (leds.effectDimension != 2) {
          leds.effectDimension = 2;
          leds.doMap = true;
          leds.fixture->doMap = true;
        }
      }
      else if (strstr(effects[leds.fx]->name(), "3D")) {
        if (leds.effectDimension != 3) {
          leds.effectDimension = 3;
          leds.doMap = true;
          leds.fixture->doMap = true;
        }
      }
      else {
        if (leds.effectDimension != 1) {
          leds.effectDimension = 1;
          leds.doMap = true;
          leds.fixture->doMap = true;
        }
      }

    } // fx < size

  }

};