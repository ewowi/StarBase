/*
   @title     StarMod
   @file      AppLeds.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
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

#include "AppFixture.h"

#include "../data/font/console_font_4x6.h"
#include "../data/font/console_font_5x8.h"
#include "../data/font/console_font_5x12.h"
#include "../data/font/console_font_6x8.h"
#include "../data/font/console_font_7x9.h"


class Fixture; //forward


//StarMod implementation of segment.data
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

};



class Leds {

public:

  uint8_t rowNr = 0;

  Fixture *fixture;

  uint16_t nrOfLeds = 64;  //amount of virtual leds (calculated by projection)

  Coord3D size = {8,8,1}; //not 0,0,0 to prevent div0 eg in Octopus2D

  uint8_t fx = -1;
  uint8_t projectionNr = -1;
  uint8_t effectDimension = -1;
  Coord3D startPos = {0,0,0}, endPos = {7,7,0}; //default

  SharedData sharedData;

  std::vector<std::vector<uint16_t>> mappingTable;

  uint16_t XY( uint8_t x, uint8_t y) {
    return x + y * size.x;
  }
  uint16_t XYZ( uint8_t x, uint8_t y, uint8_t z) {
    return x + y * size.x + z * size.x * size.y;
  }
  uint16_t XYZ(Coord3D coord) {
    return coord.x + coord.y * size.x + coord.z * size.x * size.y;
  }

  uint16_t indexVLocal = 0; //set in operator[], used by operator=

  bool doMap = false;

  Leds(uint8_t rowNr, Fixture &fixture) {
    USER_PRINTF("Leds[%d] constructor\n", rowNr);
    this->rowNr = rowNr;
    this->fixture = &fixture;
    this->fx = 13;
    this->projectionNr = 2;
  }

  ~Leds() {
    USER_PRINTF("Leds[%d] destructor\n", rowNr);
    fadeToBlackBy(100);
    for (std::vector<std::vector<uint16_t>> ::iterator physMap=mappingTable.begin(); physMap!=mappingTable.end(); ++physMap)
      physMap->clear();
    mappingTable.clear();
  }

  // indexVLocal stored to be used by other operators
  Leds& operator[](uint16_t indexV) {
    indexVLocal = indexV;
    return *this;
  }

  Leds& operator[](Coord3D pos) {
    indexVLocal = XYZ(pos.x, pos.y, pos.z);
    return *this;
  }

  // CRGB& operator[](uint16_t indexV) {
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
  void setPixelColor(uint16_t indexV, CRGB color, uint8_t blendAmount = UINT8_MAX);
  void setPixelColor(Coord3D pixel, CRGB color, uint8_t blendAmount = UINT8_MAX) {setPixelColor(XYZ(pixel), color, blendAmount);}

  CRGB getPixelColor(uint16_t indexV);
  CRGB getPixelColor(Coord3D pixel) {return getPixelColor(XYZ(pixel));}

  void addPixelColor(uint16_t indexV, CRGB color) {setPixelColor(indexV, getPixelColor(indexV) + color);}
  void addPixelColor(Coord3D pixel, CRGB color) {setPixelColor(pixel, getPixelColor(pixel) + color);}

  void fadeToBlackBy(uint8_t fadeBy = 255);
  void fill_solid(const struct CRGB& color);

  void fill_rainbow(uint8_t initialhue, uint8_t deltahue);

  void blur2d(fract8 blur_amount)
  {
      blurRows(size.x, size.y, blur_amount);
      blurColumns(size.x, size.y, blur_amount);
  }

  void blurRows(uint8_t width, uint8_t height, fract8 blur_amount)
  {
  /*    for( uint8_t row = 0; row < height; row++) {
          CRGB* rowbase = leds + (row * width);
          blur1d( rowbase, width, blur_amount);
      }
  */
      // blur rows same as columns, for irregular matrix
      uint8_t keep = 255 - blur_amount;
      uint8_t seep = blur_amount >> 1;
      for( uint8_t row = 0; row < height; row++) {
          CRGB carryover = CRGB::Black;
          for( uint8_t i = 0; i < width; i++) {
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
  void blurColumns(uint8_t width, uint8_t height, fract8 blur_amount)
  {
      // blur columns
      uint8_t keep = 255 - blur_amount;
      uint8_t seep = blur_amount >> 1;
      for( uint8_t col = 0; col < width; ++col) {
          CRGB carryover = CRGB::Black;
          for( uint8_t i = 0; i < height; ++i) {
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
  void drawCharacter(unsigned char chr, int x = 0, int16_t y = 0, uint8_t font = 0, CRGB col = CRGB::Red, uint8_t shiftPixel = 0, uint8_t shiftChr = 0) {
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
        uint8_t bits = 0;
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

  void drawText(const char * text, int x = 0, int16_t y = 0, uint8_t font = 0, CRGB col = CRGB::Red, u_int16_t shiftPixel = 0) {
    const int numberOfChr = strlen(text); //Core  1 panic'ed (LoadProhibited). Exception was unhandled. - /builds/idf/crosstool-NG/.build/HOST-x86_64-apple-darwin12/xtensa-esp32-elf/src/newlib/newlib/libc/machine/xtensa/strlen.S:82
    for (int shiftChr = 0; shiftChr < numberOfChr; shiftChr++) {
      drawCharacter(text[shiftChr], x, y, font, col, shiftPixel, shiftChr);
    }
  }

};