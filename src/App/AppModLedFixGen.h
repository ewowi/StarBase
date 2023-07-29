/*
   @title     StarMod
   @file      AppModLedFixGen.h
   @date      20230729
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "Module.h"

class AppModLedFixGen:public Module {

public:

  AppModLedFixGen() :Module("LedFixGen") {};

  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initModule(parentObject, name);

    enum Fixtures
    {
      f_1DSpiral,
      f_2DM88,
      f_2DR24,
      f_2DR35,
      f_3DC885,
      f_3DC888,
      f_3DGlobe,
      f_3DHumanSizedCube
    };

    ui->initSelect(parentObject, "ledFixGen", 3, false, [](JsonObject object) { //uiFun
      web->addResponse(object["id"], "label", "Ledfix generator");
      JsonArray select = web->addResponseA(object["id"], "select");
      select.add("1DSpiral"); //0
      select.add("2DMatrix88"); //1
      select.add("2DRing24"); //2
      select.add("2DRing35"); //3
      select.add("3DCube885"); //4
      select.add("3DCube888"); //5
      select.add("3DGlobe"); //6
      select.add("3DHumanSizedCube"); //7
    }, [](JsonObject object) { //chFun

      const char * name = "TT";
      uint16_t nrOfLeds = 64; //default
      uint16_t diameter = 100; //in mm

      uint16_t width = 0;
      uint16_t height = 0;
      uint16_t depth = 0;

      uint8_t fix = object["value"];
      switch (fix) {
        case f_1DSpiral:
          name = "1DSpiral";
          nrOfLeds = 64;
          break;
        case f_2DM88:
          name = "2DMatrix88";
          nrOfLeds = 64;
          break;
        case f_2DR24:
          name = "2DRing24";
          nrOfLeds = 24;
          break;
        case f_2DR35:
          name = "2DRing35";
          nrOfLeds = 35;
          break;
        case f_3DC885:
          name = "3DCube885";
          nrOfLeds = 320;
          break;
        case f_3DC888:
          name = "3DCube888";
          nrOfLeds = 512;
          break;
        case f_3DGlobe:
          name = "3DGlobe";
          nrOfLeds = 512;
          break;
        case f_3DHumanSizedCube:
          name = "3DHumanSizedCube";
          nrOfLeds = 2000;
          break;
      }

      char fileName[30] = "/";
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
        case f_1DSpiral:
        case f_2DR24:
        case f_2DR35:
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
        case f_2DM88:
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
        case f_3DC888:
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
        case f_3DC885:
        case f_3DHumanSizedCube:
          if (fix==f_3DC885) {
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
        case f_3DGlobe:
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

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();
  }
};

static AppModLedFixGen *lfg;