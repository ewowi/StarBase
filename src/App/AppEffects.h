/*
   @title     StarMod
   @file      AppEffects.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#ifdef USERMOD_WLEDAUDIO
  #include "../User/UserModWLEDAudio.h"
#endif
#ifdef USERMOD_E131
  #include "../User/UserModE131.h"
#endif

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
unsigned long call = 0; //not used at the moment (don't use in effect calculations)
unsigned long now = millis();
CRGBPalette16 palette = PartyColors_p;

class SharedData {
  private:
    byte *data;
    uint16_t index = 0;
    uint16_t bytesAllocated = 0;

  public:
  SharedData() {
    bytesAllocated = 1024;
    data = (byte*) malloc(bytesAllocated); //start with 100 bytes
  }

  void clear() {
    memset(data, 0, bytesAllocated);
  }

  void allocate(size_t size) {
    index = 0;
    if (size > bytesAllocated) {
      USER_PRINTF("realloc %d %d %d\n", index, size, bytesAllocated);
      data = (byte*)realloc(data, size);
      bytesAllocated = size;
    }
  }

  template <typename Type>
  Type* bind(int length = 1) {
    Type* returnValue = reinterpret_cast<Type*>(data + index);
    index += length * sizeof(Type); //add consumed amount of bytes, index is next byte which will be pointed to
    if (index > bytesAllocated) {
      USER_PRINTF("bind too big %d %d\n", index, bytesAllocated);
      return nullptr;
    }
    return returnValue;
  }

  bool allocated() {
    if (index>bytesAllocated) {
      USER_PRINTF("not all variables bound %d %d\n", index, bytesAllocated);
      return false;
    }
    return true;
  }

} sharedData;

//should not contain variables/bytes to keep mem as small as possible!!
class Effect {
public:
  virtual const char * name() {return nullptr;}

  virtual void setup() {}

  virtual void loop() {}

  virtual bool controls(JsonObject parentVar) {return false;}
};

class SolidEffect: public Effect {
public:
  const char * name() {
    return "Solid 1D";
  }
  void loop() {
    uint8_t red = mdl->getValue("Red");
    uint8_t green = mdl->getValue("Green");
    uint8_t blue = mdl->getValue("Blue");
    CRGB color = CRGB(red, green, blue);
    fill_solid( ledsP, ledsV.nrOfLedsP, color);
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "Red", 182);
    ui->initSlider(parentVar, "Green", 15);
    ui->initSlider(parentVar, "Blue", 98);
    return true;
  }
};

class RainbowEffect: public Effect {
public:
  const char * name() {
    return "Rainbow 1D";
  }
  void loop() {
    // FastLED's built-in rainbow generator
    fill_rainbow( ledsP, ledsV.nrOfLedsP, gHue, 7);
  }
};

class RainbowWithGlitterEffect:public RainbowEffect {
public:
  const char * name() {
    return "Rainbow with glitter 1D";
  }
  void loop() {
    // built-in FastLED rainbow, plus some random sparkly glitter
    RainbowEffect::loop();
    addGlitter(80);
  }
  void addGlitter( fract8 chanceOfGlitter) 
  {
    if( random8() < chanceOfGlitter) {
      ledsP[ random16(ledsV.nrOfLedsP) ] += CRGB::White;
    }
  }
};

class SinelonEffect: public Effect {
public:
  const char * name() {
    return "Sinelon 1D";
  }
  void loop() {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( ledsP, ledsV.nrOfLedsP, 20);
    int pos = beatsin16( mdl->getValue("BPM").as<int>(), 0, ledsV.nrOfLedsV-1 );
    // ledsV[pos] += CHSV( gHue, 255, 192);
    ledsV[pos] = ledsV.getPixelColor(pos) + CHSV( gHue, 255, 192);
    // CRGB x = ledsV[pos];
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "BPM", 60);
    return true;
  }
}; //Sinelon

//https://www.perfectcircuit.com/signal/difference-between-waveforms
class RunningEffect: public Effect {
public:
  const char * name() {
    return "Running 1D";
  }
  void loop() {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( ledsP, ledsV.nrOfLedsP, mdl->getValue("fade").as<int>()); //physical leds
    int pos = map(beat16( mdl->getValue("BPM").as<int>()), 0, uint16_t(-1), 0, ledsV.nrOfLedsV-1 ); //instead of call%ledsV.nrOfLedsV
    // int pos2 = map(beat16( mdl->getValue("BPM").as<int>(), 1000), 0, uint16_t(-1), 0, ledsV.nrOfLedsV-1 ); //one second later
    ledsV[pos] = CHSV( gHue, 255, 192); //make sure the right physical leds get their value
    // ledsV[ledsV.nrOfLedsV -1 - pos2] = CHSV( gHue, 255, 192); //make sure the right physical leds get their value
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "BPM", 60, 0, 255, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "in BPM!");
    });
    ui->initSlider(parentVar, "fade", 128);
    return true;
  }
};

class ConfettiEffect: public Effect {
public:
  const char * name() {
    return "Confetti 1D";
  }
  void loop() {
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy( ledsP, ledsV.nrOfLedsP, 10);
    int pos = random16(ledsV.nrOfLedsP);
    ledsP[pos] += CHSV( gHue + random8(64), 200, 255);
  }
};

class BPMEffect: public Effect {
public:
  const char * name() {
    return "Beats per minute 1D";
  }

  void loop() {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < ledsV.nrOfLedsV; i++) { //9948
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
  void loop() {
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy( ledsP, ledsV.nrOfLedsP, 20);
    uint8_t dothue = 0;
    for( int i = 0; i < 8; i++) {
      ledsP[beatsin16( i+7, 0, ledsV.nrOfLedsP-1 )] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
};

class Ripples3DEffect: public Effect {
public:
  const char * name() {
    return "Ripples 3D";
  }
  void loop() {
    uint8_t interval = mdl->getValue("interval");

    float ripple_interval = 1.3 * (interval/128.0);

    fill_solid(ledsP, ledsV.nrOfLedsP, CRGB::Black);
    // fill(CRGB::Black);

    uint16_t mW = ledsV.widthV;
    uint16_t mH = ledsV.heightV;
    uint16_t mD = ledsV.depthV;

    for (int z=0; z<mD; z++) {
        for (int x=0; x<mW; x++) {
            float d = ledsV.distance(3.5, 3.5, 0, x, z, 0)/9.899495*mH;
            uint16_t height = floor(mH/2.0+sinf(d/ripple_interval + now/100/((256.0-128.0)/20.0))*mH/2.0); //between 0 and 8

            ledsV[ledsV.XYZ(x, height, z)] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
        }
    }
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "interval", 128);
    return true;
  }
};

class SphereMove3DEffect: public Effect {
public:
  const char * name() {
    return "SphereMove 3D";
  }
  void loop() {
    uint16_t origin_x, origin_y, origin_z, d;
    float diameter;

    fill_solid(ledsP, ledsV.nrOfLedsP, CRGB::Black);
    // fill(CRGB::Black);

    uint32_t interval = now/100/((256.0-128.0)/20.0);

    uint16_t mW = ledsV.widthV;
    uint16_t mH = ledsV.heightV;
    uint16_t mD = ledsV.depthV;

    origin_x = 3.5+sinf(interval)*2.5;
    origin_y = 3.5+cosf(interval)*2.5;
    origin_z = 3.5+cosf(interval)*2.0;

    diameter = 2.0+sinf(interval/3.0);

    // CRGBPalette256 pal;
    for (int x=0; x<mW; x++) {
        for (int y=0; y<mH; y++) {
            for (int z=0; z<mD; z++) {
                d = ledsV.distance(x, y, z, origin_x, origin_y, origin_z);

                if (d>diameter && d<diameter+1) {
                  ledsV[ledsV.XYZ(x, ledsV.heightV, z)] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(pal,call, bri, LINEARBLEND);
                }
            }
        }
    }
  }
}; // SphereMove3DEffect

//XY used by blur2d
uint16_t XY( uint8_t x, uint8_t y) {
  return ledsV.XY(x,y);
}

//Frizzles2D inspired by WLED, Stepko, Andrew Tuline, https://editor.soulmatelights.com/gallery/640-color-frizzles
class Frizzles2D: public Effect {
public:
  const char * name() {
    return "Frizzles 2D";
  }

  void loop() {
    fadeToBlackBy( ledsP, ledsV.nrOfLedsP, 16);

    for (size_t i = 8; i > 0; i--) {
      uint8_t x = beatsin8(mdl->getValue("BPM").as<int>()/8 + i, 0, ledsV.widthV - 1);
      uint8_t y = beatsin8(mdl->getValue("intensity").as<int>()/8 - i, 0, ledsV.heightV - 1);
      CRGB color = ColorFromPalette(palette, beatsin8(12, 0, 255), 255);
      ledsV[XY(x,y)] = color;
    }
    blur2d(ledsP, ledsV.widthP, ledsV.heightP, mdl->getValue("blur")); //this is tricky as FastLed is not aware of our virtual 
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "BPM", 60);
    ui->initSlider(parentVar, "intensity", 128);
    ui->initSlider(parentVar, "blur", 128);
    return true;
  }
}; // Frizzles2D

class Lines2D: public Effect {
public:
  const char * name() {
    return "Lines 2D";
  }

  void loop() {
    fadeToBlackBy( ledsP, ledsV.nrOfLedsP, 100);

    if (mdl->getValue("Vertical").as<bool>()) {
      size_t x = map(beat16( mdl->getValue("BPM").as<int>()), 0, uint16_t(-1), 0, ledsV.widthV-1 ); //instead of call%width

      for (size_t y = 0; y <  ledsV.heightV; y++) {
        ledsV[XY(x,y)] = CHSV( gHue, 255, 192);
      }
    } else {
      size_t y = map(beat16( mdl->getValue("BPM").as<int>()), 0, uint16_t(-1), 0, ledsV.heightV-1 ); //instead of call%height
      for (size_t x = 0; x <  ledsV.widthV; x++) {
        ledsV[XY(x,y)] = CHSV( gHue, 255, 192);
      }
    }
  }

  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "BPM", 60);
    ui->initCheckBox(parentVar, "Vertical");
    return true;
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

  void loop() {
    const uint16_t cols = ledsV.widthV;
    const uint16_t rows = ledsV.heightV;

    uint8_t speed = mdl->getValue("speed").as<int>()/32;
    uint8_t scale = mdl->getValue("scale").as<int>()/32;

    uint8_t  w = 2;

    uint16_t a  = now/32;
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

        ledsV[XY(x,y)] = CRGB(valueR, valueG, valueB);
      }
    }
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", 128);
    ui->initSlider(parentVar, "scale", 128);
    return true;
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

  void loop() {

    const uint16_t cols = ledsV.widthV;
    const uint16_t rows = ledsV.heightV;
    const uint8_t mapp = 180 / max(cols,rows);

    uint8_t speed = mdl->getValue("speed");
    uint8_t offsetX = mdl->getValue("Offset X");
    uint8_t offsetY = mdl->getValue("Offset Y");
    uint8_t legs = mdl->getValue("Legs");

    sharedData.allocate(sizeof(map_t) * cols * rows + 2 * sizeof(uint8_t) + 2 * sizeof(uint16_t) + sizeof(uint32_t));
    map_t *rMap = sharedData.bind<map_t>(cols * rows); //array
    uint8_t *offsX = sharedData.bind<uint8_t>();
    uint8_t *offsY = sharedData.bind<uint8_t>();
    uint16_t *aux0 = sharedData.bind<uint16_t>();
    uint16_t *aux1 = sharedData.bind<uint16_t>();
    uint32_t *step = sharedData.bind<uint32_t>();
    if (!sharedData.allocated()) return;

    // re-init if SEGMENT dimensions or offset changed
    if (*aux0 != cols || *aux1 != rows || offsetX != *offsX || offsetY != *offsY) {
      // *step = 0;
      *aux0 = cols;
      *aux1 = rows;
      *offsX = offsetX;
      *offsY = offsetY;
      const uint8_t C_X = cols / 2 + (offsetX - 128)*cols/255;
      const uint8_t C_Y = rows / 2 + (offsetY - 128)*rows/255;
      for (int x = 0; x < cols; x++) {
        for (int y = 0; y < rows; y++) {
          rMap[XY(x, y)].angle = 40.7436f * atan2f(y - C_Y, x - C_X); // avoid 128*atan2()/PI
          rMap[XY(x, y)].radius = hypotf(x - C_X, y - C_Y) * mapp; //thanks Sutaburosu
        }
      }
    }

    *step = now * speed / 32 / 10;//mdl->getValue("realFps").as<int>();  // WLEDMM 40fps

    for (int x = 0; x < cols; x++) {
      for (int y = 0; y < rows; y++) {
        byte angle = rMap[XY(x,y)].angle;
        byte radius = rMap[XY(x,y)].radius;
        //CRGB c = CHSV(SEGENV.step / 2 - radius, 255, sin8(sin8((angle * 4 - radius) / 4 + SEGENV.step) + radius - SEGENV.step * 2 + angle * (SEGMENT.custom3/3+1)));
        uint16_t intensity = sin8(sin8((angle * 4 - radius) / 4 + *step/2) + radius - *step + angle * legs);
        intensity = map(intensity*intensity, 0, 65535, 0, 255); // add a bit of non-linearity for cleaner display
        CRGB color = ColorFromPalette(palette, *step / 2 - radius, intensity);
        ledsV[XY(x,y)] = color;
      }
    }
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", 128, 1, 255); //start with speed 1
    ui->initSlider(parentVar, "Offset X", 128);
    ui->initSlider(parentVar, "Offset Y", 128);
    ui->initSlider(parentVar, "Legs", 4, 1, 8);
    return true;
  }
}; // Octopus2D

//Lissajous2D inspired by WLED, Andrew Tuline 
class Lissajous2D: public Effect {
public:
  const char * name() {
    return "Lissajous 2D";
  }

  void loop() {

    const uint16_t cols = ledsV.widthV;
    const uint16_t rows = ledsV.heightV;

    uint8_t freqX = mdl->getValue("X frequency");
    uint8_t fadeRate = mdl->getValue("Fade rate");
    uint8_t speed = mdl->getValue("Speed");
    bool smooth = mdl->getValue("Smooth");

    fadeToBlackBy( ledsP, ledsV.nrOfLedsP, fadeRate);

    uint_fast16_t phase = now * speed / 256;  // allow user to control rotation speed, speed between 0 and 255!

    if (smooth) { // WLEDMM: this is the original "float" code featuring anti-aliasing
        int maxLoops = max(192, 4*(cols+rows));
        maxLoops = ((maxLoops / 128) +1) * 128; // make sure whe have half or full turns => multiples of 128
        for (int i=0; i < maxLoops; i ++) {
          float xlocn = float(sin8(phase/2 + (i* freqX)/64)) / 255.0f;  // WLEDMM align speed with original effect
          float ylocn = float(cos8(phase/2 + i*2)) / 255.0f;
          //SEGMENT.setPixelColorXY(xlocn, ylocn, SEGMENT.color_from_palette(strip.now/100+i, false, PALETTE_SOLID_WRAP, 0)); // draw pixel with anti-aliasing
          unsigned palIndex = (256*ylocn) + phase/2 + (i* freqX)/64;
          // SEGMENT.setPixelColorXY(xlocn, ylocn, SEGMENT.color_from_palette(palIndex, false, PALETTE_SOLID_WRAP, 0)); // draw pixel with anti-aliasing - color follows rotation
          ledsV[XY(xlocn, ylocn)] = ColorFromPalette(palette, palIndex);
        }
    } else
    for (int i=0; i < 256; i ++) {
      //WLEDMM: stick to the original calculations of xlocn and ylocn
      uint_fast8_t xlocn = sin8(phase/2 + (i*freqX)/64);
      uint_fast8_t ylocn = cos8(phase/2 + i*2);
      xlocn = (cols < 2) ? 1 : (map(2*xlocn, 0,511, 0,2*(cols-1)) +1) /2;    // softhack007: "*2 +1" for proper rounding
      ylocn = (rows < 2) ? 1 : (map(2*ylocn, 0,511, 0,2*(rows-1)) +1) /2;    // "rows > 2" is needed to avoid div/0 in map()
      // SEGMENT.setPixelColorXY((uint8_t)xlocn, (uint8_t)ylocn, SEGMENT.color_from_palette(strip.now/100+i, false, PALETTE_SOLID_WRAP, 0));
      ledsV[XY(xlocn, ylocn)] = ColorFromPalette(palette, now/100+i);
    }
  }
  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "X frequency", 64);
    ui->initSlider(parentVar, "Fade rate", 128);
    ui->initSlider(parentVar, "Speed", 128);
    ui->initCheckBox(parentVar, "Smooth");
    return true;
  }
}; // Lissajous2D


#define maxNumBalls 16

class BouncingBalls1D: public Effect {
public:
  //BouncingBalls1D  inspired by WLED
  //each needs 12 bytes
  typedef struct Ball {
    unsigned long lastBounceTime;
    float impactVelocity;
    float height;
  } ball;

  const char * name() {
    return "Bouncing Balls 1D";
  }

  void loop() {
    uint8_t grav = mdl->getValue("gravity");
    uint8_t numBalls = mdl->getValue("balls");

    sharedData.allocate(sizeof(Ball) * maxNumBalls);
    Ball *balls = sharedData.bind<Ball>(maxNumBalls); //array
    if (!sharedData.allocated()) return;

    fill_solid(ledsP, ledsV.nrOfLedsP, CRGB::Black);

    // non-chosen color is a random color
    const float gravity = -9.81f; // standard value of gravity
    // const bool hasCol2 = SEGCOLOR(2);
    const unsigned long time = now;

    //not necessary as sharedData is cleared at setup()
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

      int pos = roundf(balls[i].height * (ledsV.nrOfLedsV - 1));

      CRGB color = ColorFromPalette(palette, i*(256/max(numBalls, (uint8_t)8)), 255);

      ledsV[pos] = color;
      // if (SEGLEN<32) SEGMENT.setPixelColor(indexToVStrip(pos, stripNr), color); // encode virtual strip into index
      // else           SEGMENT.setPixelColor(balls[i].height + (stripNr+1)*10.0f, color);
    } //balls
  }

  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "gravity", 128);
    ui->initSlider(parentVar, "balls", 8, 1, 16);
    return true;
  }
}; // BouncingBalls2D

class RingEffect:public Effect {
  protected:

    void setRing(int ring, CRGB colour) { //so britisch ;-)
      ledsV[ring] = colour;
    }

};

class RingRandomFlow:public RingEffect {
public:
  const char * name() {
    return "RingRandomFlow 1D";
  }

  void loop() {
    sharedData.allocate(sizeof(uint8_t) * ledsV.nrOfLedsV);
    uint8_t *hue = sharedData.bind<uint8_t>(ledsV.nrOfLedsV); //array
    if (!sharedData.allocated()) return;

    hue[0] = random(0, 255);
    for (int r = 0; r < ledsV.nrOfLedsV; r++) {
      setRing(r, CHSV(hue[r], 255, 255));
    }
    for (int r = (ledsV.nrOfLedsV - 1); r >= 1; r--) {
      hue[r] = hue[(r - 1)]; // set this ruing based on the inner
    }
    // FastLED.delay(SPEED);
  }
};


#ifdef USERMOD_WLEDAUDIO

class GEQEffect:public Effect {
public:
  const char * name() {
    return "GEQ 2D";
  }

  void setup() {
    fadeToBlackBy( ledsP, ledsV.nrOfLedsP, 16);
  }

  void loop() {
    sharedData.allocate(sizeof(uint16_t) * ledsV.widthV + sizeof(uint32_t));
    uint16_t *previousBarHeight = sharedData.bind<uint16_t>(ledsV.widthV); //array
    uint32_t *step = sharedData.bind<uint32_t>();
    if (!sharedData.allocated()) return;

    const int NUM_BANDS = NUM_GEQ_CHANNELS ; // map(SEGMENT.custom1, 0, 255, 1, 16);
    const uint16_t cols = ledsV.widthV;
    const uint16_t rows = ledsV.heightV; 

    uint8_t *fftResult = wledAudioMod->fftResults;
    #ifdef SR_DEBUG
    uint8_t samplePeak = *(uint8_t*)um_data->u_data[3];
    #endif

    uint8_t fadeOut = mdl->getValue("fadeOut");
    uint8_t ripple = mdl->getValue("ripple"); 
    bool colorBars = mdl->getValue("colorBars");
    bool smoothBars = mdl->getValue("smoothBars");

    bool rippleTime = false;
    if (now - *step >= (256U - ripple)) {
      *step = now;
      rippleTime = true;
    }

    int fadeoutDelay = (256 - fadeOut) / 64; //256..1 -> 4..0
    size_t beat = map(beat16( fadeOut), 0, uint16_t(-1), 0, fadeoutDelay-1 ); // instead of call%fadeOutDelay

    if ((fadeoutDelay <= 1 ) || (beat == 0)) fadeToBlackBy( ledsP, ledsV.nrOfLedsP, fadeOut);

    uint16_t lastBandHeight = 0;  // WLEDMM: for smoothing out bars

    //WLEDMM: evenly ditribut bands
    float bandwidth = (float)cols / NUM_BANDS;
    float remaining = bandwidth;
    uint8_t band = 0;
    for (int x=0; x < cols; x++) {
      //WLEDMM if not enough remaining
      if (remaining < 1) {band++; remaining+= bandwidth;} //increase remaining but keep the current remaining
      remaining--; //consume remaining

      // USER_PRINTF("x %d b %d n %d w %f %f\n", x, band, NUM_BANDS, bandwidth, remaining);
      uint8_t frBand = ((NUM_BANDS < 16) && (NUM_BANDS > 1)) ? map(band, 0, NUM_BANDS - 1, 0, 15):band; // always use full range. comment out this line to get the previous behaviour.
      // frBand = constrain(frBand, 0, 15); //WLEDMM can never be out of bounds (I think...)
      uint16_t colorIndex = frBand * 17; //WLEDMM 0.255
      uint16_t bandHeight = fftResult[frBand];  // WLEDMM we use the original ffResult, to preserve accuracy

      // WLEDMM begin - smooth out bars
      if ((x > 0) && (x < (cols-1)) && (smoothBars)) {
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
        if (colorBars) //color_vertical / color bars toggle
          colorIndex = map(y, 0, rows-1, 0, 255);

        ledColor = ColorFromPalette(palette, (uint8_t)colorIndex);

        ledsV.setPixelColor(XY(x, rows - 1 - y), ledColor);
      }

      if ((ripple > 0) && (previousBarHeight[x] > 0) && (previousBarHeight[x] < rows))  // WLEDMM avoid "overshooting" into other segments
        ledsV.setPixelColor(XY(x, rows - previousBarHeight[x]), CHSV( gHue, 255, 192)); // take gHue color for the time being

      if (rippleTime && previousBarHeight[x]>0) previousBarHeight[x]--;    //delay/ripple effect

    }
  }

  bool controls(JsonObject parentVar) {
    ui->initSlider(parentVar, "fadeOut", 255);
    ui->initSlider(parentVar, "ripple", 128);
    ui->initCheckBox(parentVar, "colorBars");
    ui->initCheckBox(parentVar, "smoothBars", true); //default on

    // Nice an effect can register it's own DMX channel, but not a fan of repeating the range and type of the param

    #ifdef USERMOD_E131

      if (e131mod->isEnabled) {
        e131mod->patchChannel(3, "fadeOut", 255); // TODO: add constant for name
        e131mod->patchChannel(4, "ripple", 255);
      }

    #endif

    return true;
  }
};

class AudioRings:public RingEffect {
public:
  const char * name() {
    return "AudioRings 1D";
  }

  void loop() {
    uint8_t *fftResult = wledAudioMod->fftResults;

    for (int i = 0; i < 7; i++) { // 7 rings

      uint8_t val;
      if(mdl->getValue("inWards").as<bool>()) {
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
  void setRingFromFtt(int index, int ring) {
    uint8_t *fftResult = wledAudioMod->fftResults;
    uint8_t val = fftResult[index];
    // Visualize leds to the beat
    CRGB color = ColorFromPalette(palette, val, 255);
    color.nscale8_video(val);
    setRing(ring, color);
  }

  bool controls(JsonObject parentVar) {
    ui->initCheckBox(parentVar, "inWards");
    return true;
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
    #ifdef USERMOD_WLEDAUDIO
      effects.push_back(new GEQEffect);
      effects.push_back(new AudioRings);
    #endif
  }

  void setup() {
    //check of no local variables (should be only 4 bytes): tbd: can we loop over effects (sizeof(effect does not work))
    // for (Effect *effect:effects) {
    //     USER_PRINTF("Size of %s is %d\n", effect->name(), sizeof(*effect));
    // }
    USER_PRINTF("Size of %s is %d\n", "RainbowEffect", sizeof(RainbowEffect));
    USER_PRINTF("Size of %s is %d\n", "RainbowWithGlitterEffect", sizeof(RainbowWithGlitterEffect));
    USER_PRINTF("Size of %s is %d\n", "SinelonEffect", sizeof(SinelonEffect));
    USER_PRINTF("Size of %s is %d\n", "RunningEffect", sizeof(RunningEffect));
    USER_PRINTF("Size of %s is %d\n", "ConfettiEffect", sizeof(ConfettiEffect));
    USER_PRINTF("Size of %s is %d\n", "BPMEffect", sizeof(BPMEffect));
    USER_PRINTF("Size of %s is %d\n", "JuggleEffect", sizeof(JuggleEffect));
    USER_PRINTF("Size of %s is %d\n", "Ripples3DEffect", sizeof(Ripples3DEffect));
    USER_PRINTF("Size of %s is %d\n", "SphereMove3DEffect", sizeof(SphereMove3DEffect));
    USER_PRINTF("Size of %s is %d\n", "Frizzles2D", sizeof(Frizzles2D));
    USER_PRINTF("Size of %s is %d\n", "Lines2D", sizeof(Lines2D));
    USER_PRINTF("Size of %s is %d\n", "DistortionWaves2D", sizeof(DistortionWaves2D));
    USER_PRINTF("Size of %s is %d\n", "Octopus2D", sizeof(Octopus2D));
    USER_PRINTF("Size of %s is %d\n", "Lissajous2D", sizeof(Lissajous2D));
    USER_PRINTF("Size of %s is %d\n", "BouncingBalls1D", sizeof(BouncingBalls1D));
    USER_PRINTF("Size of %s is %d\n", "RingRandomFlow", sizeof(RingRandomFlow));
    #ifdef USERMOD_WLEDAUDIO
      USER_PRINTF("Size of %s is %d\n", "GEQEffect", sizeof(GEQEffect));
      USER_PRINTF("Size of %s is %d\n", "AudioRings", sizeof(AudioRings));
    #endif
  }

  void loop(size_t index) {
    now = millis(); //tbd timebase

    effects[index%effects.size()]->loop();

    call++;

    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }

  size_t size() {
    return effects.size();
  }

  bool setEffect(const char * id, size_t index) {
    bool doMap = false;

    USER_PRINTF("setEffect %d %d %d \n", index, effects.size(), size());
    if (index < size()) {

      //tbd: make property of effects
      if (strstr(effects[index]->name(), "2D")) {
        if (ledsV.effectDimension != 2) {
          ledsV.effectDimension = 2;
          doMap = true;
        }
      }
      else if (strstr(effects[index]->name(), "3D")) {
        if (ledsV.effectDimension != 3) {
          ledsV.effectDimension = 3;
          doMap = true;
        }
      }
      else {
        if (ledsV.effectDimension != 1) {
          ledsV.effectDimension = 1;
          doMap = true;
        }
      }

      sharedData.clear(); //make sure all values are 0

      JsonObject parentVar = mdl->findVar(id);
      parentVar.remove("n"); //tbd: we should also remove the uiFun and chFun !!

      Effect* effect = effects[index];
      effect->controls(parentVar);
      effect->setup(); //if changed then run setup once (like call==0 in WLED)

      print->printJson("parentVar", parentVar);
      web->sendDataWs(parentVar); //always send, also when no children, to remove them from ui
    } // fx < size

    return doMap;
  }

};