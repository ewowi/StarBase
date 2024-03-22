/*
   @title     StarMod
   @file      LedLeds.h
   @date      20240227
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
// FastLED optional flags to configure drivers, see https://github.com/FastLED/FastLED/blob/master/src/platforms/esp/32
// RMT driver (default)
// #define FASTLED_ESP32_FLASH_LOCK 1    // temporarily disabled FLASH file access while driving LEDs (may prevent random flicker)
// #define FASTLED_RMT_BUILTIN_DRIVER 1  // in case your app needs to use RMT units, too (slower)
// I2S parallel driver
// #define FASTLED_ESP32_I2S true        // to use I2S parallel driver (instead of RMT)
// #define I2S_DEVICE 1                  // I2S driver: allows to still use I2S#0 for audio (only on esp32 and esp32-s3)
// #define FASTLED_I2S_MAX_CONTROLLERS 8 // 8 LED pins should be enough (default = 24)
#include "FastLED.h"

#include "LedFixture.h"

#include "../data/font/console_font_4x6.h"
#include "../data/font/console_font_5x8.h"
#include "../data/font/console_font_5x12.h"
#include "../data/font/console_font_6x8.h"
#include "../data/font/console_font_7x9.h"

enum Projections
{
  p_Default,
  p_Multiply,
  p_Rotate,
  p_DistanceFromPoint,
  p_Preset1,
  p_None,
  p_Random,
  p_Reverse,
  p_Mirror,
  p_Kaleidoscope,
  p_count
};

#define trigoInt8 0
#define trigoInt16 1 //default
#define trigoFloat 2
//     sin8/cos8   sin16/cos16
//0:   128, 255    0 32645
//64:  255, 128    32645 0
//128: 128, 1      0 -32645
//192: 1, 127      -32645 0

struct Trigo {
  float sinValue; uint16_t sinAngle = UINT16_MAX; //caching of sinValue=sin(sinAngle)
  float cosValue; uint16_t cosAngle = UINT16_MAX; //caching of cosValue=cos(cosAngle)
  uint8_t type = trigoInt16;
  Trigo(uint8_t type = trigoInt16) {
    this->type = type;
  }
  int16_t sin(int16_t factor, uint16_t angle, uint16_t period = 360) {
    if (sinAngle != angle) {
      sinAngle = angle;
      sinValue = type==trigoInt16?sin16(65536.0f * angle / period) / 32645.0f:
                 type==trigoInt8?(sin8(256.0f * angle / period) - 128) / 127.0f:
                 sinf(DEG_TO_RAD * 360 * angle / period);
    } else USER_PRINTF("%d", type); //debug show cache efficiency
    return factor * sinValue;
  }
  int16_t cos(int16_t factor, uint16_t angle, uint16_t period = 360) {
    if (cosAngle != angle) {
      cosAngle = angle;
      cosValue = type==trigoInt16?cos16(65536.0f * angle / period) / 32645.0f:
                 type==trigoInt8?(cos8(256.0f * angle / period) - 128) / 127.0f:
                 cosf(DEG_TO_RAD * 360 * angle / period);
    } else USER_PRINTF("%d", type); //debug show cache efficiency
    return factor * cosValue;
  }
  // https://msl.cs.uiuc.edu/planning/node102.html
  Coord3D rotateRoll(Coord3D in, Coord3D middle, uint16_t roll) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = cos(inM.x, roll) - sin(inM.y, roll);
    out.y = sin(inM.x, roll) + cos(inM.y, roll);
    out.z = inM.z;
    return out + middle;
  }
  Coord3D rotatePan(Coord3D in, Coord3D middle, uint16_t pan) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = cos(inM.x, pan) + sin(inM.z, pan);
    out.y = inM.y;
    out.z = - sin(inM.x, pan) + cos(inM.z, pan);
    return out + middle;
  }
  Coord3D rotateTilt(Coord3D in, Coord3D middle, uint16_t tilt) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = inM.x;
    out.y = cos(inM.y, tilt) - sin(inM.z, tilt);
    out.z = sin(inM.y, tilt) + cos(inM.z, tilt);
    return out + middle;
  }
  Coord3D rotate(Coord3D in, Coord3D middle, uint16_t pan, uint16_t tilt, uint16_t roll) {
    return rotateRoll(rotateTilt(rotatePan(in, middle, pan), middle, tilt), middle, roll);
  }
};


class Fixture; //forward



//StarMod implementation of segment.data
class SharedData {

  private:
    byte *data;
    unsigned16 index = 0;
    unsigned16 bytesAllocated = 0;

  public:

  SharedData() {
    USER_PRINTF("SharedData constructor %d %d\n", index, bytesAllocated);
  }
  ~SharedData() {
    USER_PRINTF("SharedData destructor WIP %d %d\n", index, bytesAllocated);
    // free(data);
  }

  void clear() {
    memset(data, 0, bytesAllocated);
    index = 0;
  }

  void loop() {
    index = 0;
  }

  template <typename Type>
  Type * bind(Type * returnValue, int length = 1) {
    size_t newIndex = index + length * sizeof(Type);
    if (newIndex > bytesAllocated) {
      size_t newSize = bytesAllocated + (1 + ( newIndex - bytesAllocated)/1024) * 1024; // add a multitude of 1024 bytes
      USER_PRINTF("bind add more %d->%d %d->%d\n", index, newIndex, bytesAllocated, newSize);
      if (bytesAllocated == 0)
        data = (byte*) malloc(newSize);
      else
        data = (byte*)realloc(data, newSize);
      bytesAllocated = newSize;
    }
    // USER_PRINTF("bind %d->%d %d\n", index, newIndex, bytesAllocated);
    returnValue = reinterpret_cast<Type *>(data + index);
    index = newIndex; //add consumed amount of bytes, index is next byte which will be pointed to
    return returnValue;
  }

};

static float sinrot = 0.0f;
static float cosrot = 1.0f;
static unsigned long last_millis = UINT_MAX;

constexpr float projScaleMax = 1.0f;   // full size
constexpr float projScaleMin = 0.701f; // 1/sqrt(2)
static float projScale = projScaleMax;

static Coord3D spinXY(uint_fast16_t x, uint_fast16_t y, uint_fast16_t width, uint_fast16_t height, unsigned8 speed) {
  if ((millis()/12) !=  last_millis) {
    // update sin / cos for rotation - once each 12ms
    float now = float(millis()/12) / (255 - speed);  // this sets the rotation speed
    //float now = float(strip.now) / 2000.0f;  // error: 'strip' was not declared in this scope
    sinrot = sinf(now);
    cosrot = cosf(now);
    last_millis = millis()/12;
    // scale to fit - comment out the next lines to disable
    float maxProj = max(abs(width/2 * sinrot), abs(height/2 * cosrot));
    int maxdim = max(width/2, height/2);
    float newScaling = maxProj / float(maxdim);
    projScale = max(min(newScaling, projScaleMax), projScaleMin);
  }
  // center
  int x1 = int(x) - width/2;
  int y1 = int(y) - height/2;
  // matrix mult for rotation
  float x2 = float(x1) * cosrot - float(y1) * sinrot;
  float y2 = float(x1) * sinrot + float(y1) * cosrot;
  // un-center
  int x3 = lround(x2 * projScale) + width/2;  // projScale adds some down-scaling,
  int y3 = lround(y2 * projScale) + height/2; //     so everything fits fully into the original matrix. Note to self: this is still sub-optimal.
  // check bounds

  if ((x3 <0) || (x3 >= width) || (y3 <0) || (y3 >= height)) return Coord3D{0, 0, 0}; // outside of matrix
  // deliver fish
  else return Coord3D{(unsigned16)x3, (unsigned16)y3, 0};
}

struct PhysMap {
  // bool isPhys = false; // 1 byte
  // union {
    std::vector<unsigned16> * indexes;
    CRGB color;
  // }; // 4 bytes
}; // expected to be 5 bytes but is 8 bytes!!!

class Leds {

public:

  Fixture *fixture;

  unsigned16 nrOfLeds = 64;  //amount of virtual leds (calculated by projection)

  Coord3D size = {8,8,1}; //not 0,0,0 to prevent div0 eg in Octopus2D

  unsigned8 fx = -1;
  unsigned8 projectionNr = -1;
  unsigned8 effectDimension = -1;

  Coord3D startPos = {0,0,0}, endPos = {UINT16_MAX,UINT16_MAX,UINT16_MAX}; //default
  unsigned8 proRSpeed = 128;

  SharedData sharedData;

  std::vector<PhysMap> mappingTable;

  unsigned16 indexVLocal = 0; //set in operator[], used by operator=

  bool doMap = false;

  unsigned16 XY(unsigned16 x, unsigned16 y) {
    return XYZ(x, y, 0);
  }

  unsigned16 XYZNoSpin(Coord3D coord) {
    return coord.x + coord.y * size.x + coord.z * size.x * size.y;
  }

  unsigned16 XYZ(Coord3D coord) {
    return XYZ(coord.x, coord.y, coord.z);
  }

  unsigned16 XYZ(unsigned16 x, unsigned16 y, unsigned16 z) {
    if (projectionNr == p_Rotate || projectionNr == p_Preset1) {
      Coord3D result = spinXY(x, y, size.x, size.y, proRSpeed);
      return result.x + result.y * size.x + result.z * size.x * size.y;
    }
    else
      return x + y * size.x + z * size.x * size.y;
  }

  Leds(Fixture &fixture) {
    USER_PRINTF("Leds[%d] constructor %d\n", UINT8_MAX, sizeof(PhysMap));
    this->fixture = &fixture;
  }

  ~Leds() {
    USER_PRINTF("Leds[%d] destructor\n", UINT8_MAX);
    fadeToBlackBy(100);
    doMap = true; // so loop is not running while deleting
    for (PhysMap &map:mappingTable) {
      if (map.indexes) {
        map.indexes->clear();
        delete map.indexes;
      }
    }
    mappingTable.clear();
  }

  // indexVLocal stored to be used by other operators
  Leds& operator[](unsigned16 indexV) {
    indexVLocal = indexV;
    return *this;
  }

  Leds& operator[](Coord3D pos) {
    indexVLocal = XYZ(pos.x, pos.y, pos.z);
    return *this;
  }

  // CRGB& operator[](unsigned16 indexV) {
  //   // indexVLocal = indexV;
  //   CRGB x = getPixelColor(indexV);
  //   return x;
  // }

  // uses indexVLocal and color to call setPixelColor
  Leds& operator=(const CRGB color) {
    setPixelColor(indexVLocal, color);
    return *this;
  }

  Leds& operator+=(const CRGB color) {
    setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
    return *this;
  }
  Leds& operator|=(const CRGB color) {
    // setPixelColor(indexVLocal, color);
    setPixelColor(indexVLocal, getPixelColor(indexVLocal) | color);
    return *this;
  }

  // Leds& operator+(const CRGB color) {
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
  //   return *this;
  // }


  // maps the virtual led to the physical led(s) and assign a color to it
  void setPixelColor(unsigned16 indexV, CRGB color, unsigned8 blendAmount = UINT8_MAX);
  void setPixelColor(Coord3D pixel, CRGB color, unsigned8 blendAmount = UINT8_MAX) {setPixelColor(XYZ(pixel), color, blendAmount);}

  CRGB getPixelColor(unsigned16 indexV);
  CRGB getPixelColor(Coord3D pixel) {return getPixelColor(XYZ(pixel));}

  void addPixelColor(unsigned16 indexV, CRGB color) {setPixelColor(indexV, getPixelColor(indexV) + color);}
  void addPixelColor(Coord3D pixel, CRGB color) {setPixelColor(pixel, getPixelColor(pixel) + color);}

  void fadeToBlackBy(unsigned8 fadeBy = 255);
  void fill_solid(const struct CRGB& color, bool noBlend = false);
  void fill_rainbow(unsigned8 initialhue, unsigned8 deltahue);

  void blur1d(fract8 blur_amount)
  {
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    CRGB carryover = CRGB::Black;
    for( uint16_t i = 0; i < nrOfLeds; ++i) {
        CRGB cur = getPixelColor(i);
        CRGB part = cur;
        part.nscale8( seep);
        cur.nscale8( keep);
        cur += carryover;
        if( i) addPixelColor(i-1, part);
        setPixelColor(i, cur);
        carryover = part;
    }
  }

  void blur2d(fract8 blur_amount)
  {
      blurRows(size.x, size.y, blur_amount);
      blurColumns(size.x, size.y, blur_amount);
  }

  void blurRows(unsigned8 width, unsigned8 height, fract8 blur_amount)
  {
  /*    for (forUnsigned8 row = 0; row < height; row++) {
          CRGB* rowbase = leds + (row * width);
          blur1d( rowbase, width, blur_amount);
      }
  */
      // blur rows same as columns, for irregular matrix
      stackUnsigned8 keep = 255 - blur_amount;
      stackUnsigned8 seep = blur_amount >> 1;
      for (forUnsigned8 row = 0; row < height; row++) {
          CRGB carryover = CRGB::Black;
          for (forUnsigned8 i = 0; i < width; i++) {
              CRGB cur = getPixelColor(XY(i,row));
              CRGB part = cur;
              part.nscale8( seep);
              cur.nscale8( keep);
              cur += carryover;
              if( i) addPixelColor(XY(i-1,row), part);
              setPixelColor(XY(i,row), cur);
              carryover = part;
          }
      }
  }

  // blurColumns: perform a blur1d on each column of a rectangular matrix
  void blurColumns(unsigned8 width, unsigned8 height, fract8 blur_amount)
  {
      // blur columns
      stackUnsigned8 keep = 255 - blur_amount;
      stackUnsigned8 seep = blur_amount >> 1;
      for (forUnsigned8 col = 0; col < width; ++col) {
          CRGB carryover = CRGB::Black;
          for (forUnsigned8 i = 0; i < height; ++i) {
              CRGB cur = getPixelColor(XY(col,i));
              CRGB part = cur;
              part.nscale8( seep);
              cur.nscale8( keep);
              cur += carryover;
              if( i) addPixelColor(XY(col,i-1), part);
              setPixelColor(XY(col,i), cur);
              carryover = part;
          }
      }
  }

  //shift is used by drawText indicating which letter it is drawing
  void drawCharacter(unsigned char chr, int x = 0, int16_t y = 0, unsigned8 font = 0, CRGB col = CRGB::Red, unsigned16 shiftPixel = 0, unsigned16 shiftChr = 0) {
    if (chr < 32 || chr > 126) return; // only ASCII 32-126 supported
    chr -= 32; // align with font table entries

    Coord3D fontSize;
    switch (font%5) {
      case 0: fontSize.x = 4; fontSize.y = 6; break;
      case 1: fontSize.x = 5; fontSize.y = 8; break;
      case 2: fontSize.x = 5; fontSize.y = 12; break;
      case 3: fontSize.x = 6; fontSize.y = 8; break;
      case 4: fontSize.x = 7; fontSize.y = 9; break;
    }

    Coord3D chrPixel;
    for (chrPixel.y = 0; chrPixel.y<fontSize.y; chrPixel.y++) { // character height
      Coord3D pixel;
      pixel.y = y + chrPixel.y;
      if (pixel.y >=0 && pixel.y < size.y) {
        byte bits = 0;
        switch (font%5) {
          case 0: bits = pgm_read_byte_near(&console_font_4x6[(chr * fontSize.y) + chrPixel.y]); break;
          case 1: bits = pgm_read_byte_near(&console_font_5x8[(chr * fontSize.y) + chrPixel.y]); break;
          case 2: bits = pgm_read_byte_near(&console_font_5x12[(chr * fontSize.y) + chrPixel.y]); break;
          case 3: bits = pgm_read_byte_near(&console_font_6x8[(chr * fontSize.y) + chrPixel.y]); break;
          case 4: bits = pgm_read_byte_near(&console_font_7x9[(chr * fontSize.y) + chrPixel.y]); break;
        }

        for (chrPixel.x = 0; chrPixel.x<fontSize.x; chrPixel.x++) {
          //x adjusted by: chr in text, scroll value, font column
          pixel.x = (x + shiftChr * fontSize.x + shiftPixel + (fontSize.x-1) - chrPixel.x)%size.x;
          if ((pixel.x >= 0 && pixel.x < size.x) && ((bits>>(chrPixel.x+(8-fontSize.x))) & 0x01)) { // bit set & drawing on-screen
            setPixelColor(pixel, col);
          }
        }
      }
    }
  }

  void drawText(const char * text, int x = 0, int16_t y = 0, unsigned8 font = 0, CRGB col = CRGB::Red, unsigned16 shiftPixel = 0) {
    const int numberOfChr = strlen(text); //Core  1 panic'ed (LoadProhibited). Exception was unhandled. - /builds/idf/crosstool-NG/.build/HOST-x86_64-apple-darwin12/xtensa-esp32-elf/src/newlib/newlib/libc/machine/xtensa/strlen.S:82
    for (int shiftChr = 0; shiftChr < numberOfChr; shiftChr++) {
      drawCharacter(text[shiftChr], x, y, font, col, shiftPixel, shiftChr);
    }
  }

};