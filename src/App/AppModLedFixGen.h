/*
   @title     StarMod
   @file      AppModLedFixGen.h
   @date      20230730
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

    parentVar = ui->initModule(parentVar, name);

    ui->initSelect(parentVar, "ledFixGen", 0, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Fixture");
      web->addResponse(var["id"], "comment", "Type of fixture");
      JsonArray select = web->addResponseA(var["id"], "select");
      select.add("1DSpiral"); //0
      select.add("2DMatrix"); //1
      select.add("2DRing"); //2
      select.add("2DRings241"); //3
      select.add("3DSideCube"); //4
      select.add("3DCube"); //5
      select.add("3DGlobe"); //6
    }, [](JsonObject var) { //chFun

      ledFixGenChFun(var);
    }); //ledFixGen

    ui->initButton(parentVar, "generate", nullptr, false, [](JsonObject var) { //uiFun
      // web->addResponse(var["id"], "comment", "All but model.json");
    }, [](JsonObject var) {

      generateChFun(var);

    });

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();
  }

  enum Fixtures
  {
    f_1DSpiral,
    f_2DMatrix,
    f_2DRing,
    f_2DRings241,
    f_3DSideCube,
    f_3DCube,
    f_3DGlobe
  };

  //generate dynamic html for fixture parameters
  static void ledFixGenChFun(JsonObject var) {
    JsonObject parentVar = mdl->findVar(var["id"]);
    parentVar.remove("n"); //tbd: we should also remove the uiFun and chFun !!
    uint8_t value = var["value"];
    
    if (value == f_1DSpiral) {
      ui->initNumber(parentVar, "ledCount", 64, false);
    }
    else if (value == f_2DRing) {
      ui->initNumber(parentVar, "ledCount", 24, false);
    }
    else if (value == f_2DRings241) {
      ui->initCheckBox(parentVar, "in2out", true, false);
    }
    else if (value == f_2DMatrix) {
      ui->initNumber(parentVar, "width", 8, false);

      ui->initNumber(parentVar, "height", 8, false);

      ui->initSelect(parentVar, "firstLedX", 0, false, [](JsonObject var) { //uiFun
        // web->addResponse(var["id"], "label", "Ledfix generator");
        JsonArray select = web->addResponseA(var["id"], "select");
        select.add("Left"); //0
        select.add("Richt"); //1
      });
      ui->initSelect(parentVar, "firstLedY", 0, false, [](JsonObject var) { //uiFun
        // web->addResponse(var["id"], "label", "Ledfix generator");
        JsonArray select = web->addResponseA(var["id"], "select");
        select.add("Top"); //0
        select.add("Bottom"); //1
      });

      ui->initCheckBox(parentVar, "serpentine", false, false);
    }
    else if (value == f_3DCube) {
      ui->initNumber(parentVar, "width", 8, false);
      ui->initNumber(parentVar, "height", 8, false);
      ui->initNumber(parentVar, "depth", 8, false);
    }
    else if (value == f_3DSideCube) {
      ui->initNumber(parentVar, "width", 8, false);
      ui->initNumber(parentVar, "sides", 5, false);
    }
    else if (value == f_3DGlobe) {
      ui->initNumber(parentVar, "width", 12, false);
    }

    web->sendDataWs(parentVar); //always send, also when no children, to remove them from ui
  }

  static void generateChFun(JsonObject var) {

    uint8_t fix = mdl->getValue("ledFixGen");

    char name[32] = "TT";
    char nameExt[12];
    uint16_t nrOfLeds = 64; //default
    uint16_t factor = 1;

    float width = 8;
    float height = 8;
    float depth = 1;

    char sep[3]="";
    char sep2[3]="";

    uint8_t pin = 10;

    switch (fix) {
      case f_1DSpiral:
        nrOfLeds = mdl->getValue("ledCount");
        strcpy(name, "1DSpiral");
        snprintf(nameExt, sizeof(nameExt), "%02d", nrOfLeds);
        strcat(name, nameExt);
        break;
      case f_2DMatrix:
        width = mdl->getValue("width");
        height = mdl->getValue("height");
        depth = 1;
        nrOfLeds = width * height;
        strcpy(name, "2DMatrix");
        snprintf(nameExt, sizeof(nameExt), "%02.0f%02.0f", width, height);
        strcat(name, nameExt);
        break;
      case f_2DRing:
        strcpy(name, "2DRing");
        nrOfLeds = mdl->getValue("ledCount");
        snprintf(nameExt, sizeof(nameExt), "%02d", nrOfLeds);
        strcat(name, nameExt);
        break;
      case f_2DRings241:
        strcpy(name, "2DRings241");
        // width = 20;
        // height = 20;
        // depth = 1;
        nrOfLeds = 241;
        break;
      case f_3DSideCube:
        width = mdl->getValue("width");
        height = width;
        depth = width;
        nrOfLeds = width * height * mdl->getValue("sides").as<int>();
        strcpy(name, "3DSideCube");
        snprintf(nameExt, sizeof(nameExt), "%02.0f%02.0f%02d", width, width, mdl->getValue("sides").as<int>());
        strcat(name, nameExt);
        break;
      case f_3DCube:
        width = mdl->getValue("width");
        height = mdl->getValue("height");;
        depth = mdl->getValue("depth");;
        nrOfLeds = width * height * depth;
        strcpy(name, "3DCube");
        snprintf(nameExt, sizeof(nameExt), "%02.0f%02.0f%02.0f", width, height, depth);
        strcat(name, nameExt);
        break;
      case f_3DGlobe:
        width = mdl->getValue("width");
        height = width;
        depth = width;
        nrOfLeds = width * width * PI;
        strcpy(name, "3DGlobe");
        snprintf(nameExt, sizeof(nameExt), "%02.0f", width);
        strcat(name, nameExt);
        break;
    }

    File f = openFile(name);

    f.print("{");
    
    f.printf("\"name\":\"%s\"", name);
    f.printf(",\"nrOfLeds\":%d", nrOfLeds);
    // f.printf(",\"pin\":%d",16);

    //tbd: use if then else and place declarations inside there
    float ringDiam;

    uint8_t rings[9] = {1, 8, 12, 16, 24, 32, 40, 48, 60};
    //  {0, 0},     //0 Center Point -> 1
    //   {1, 8},     //1 -> 8
    //   {9, 20},   //2 -> 12
    //   {21, 36},   //3 -> 16
    //   {37, 60},   //4 -> 24
    //   {61, 92},   //5 -> 32
    //   {93, 132},  //6 -> 40
    //   {133, 180}, //7 -> 48
    //   {181, 240}, //8 Outer Ring -> 60   -> 241

    int arrSize;
    bool in2out;

    switch (fix) {
      case f_1DSpiral:
        factor = 10;

        f.printf(",\"scale\":\"%s\"", "mm"); //scale currently not used (to show multiple fixtures)
        f.printf(",\"factor\":%d", factor);

        width = 10;
        height = nrOfLeds/12;
        depth = 10;
        f.printf(",\"width\":%.0f", width);
        f.printf(",\"height\":%.0f", height);
        f.printf(",\"depth\":%.0f", depth);

        f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
        strcpy(sep, "");
        for (int i=0; i<nrOfLeds; i++) {
          float radians = i*360/48 * (M_PI / 180); //48 leds per round
          uint16_t x = factor * width/2 * (1 + sinf(radians));
          uint16_t y = factor * i/12; //
          uint16_t z = factor * depth/2 * (1+ cosf(radians));
          f.printf("%s[%d,%d,%d]", sep, x, y, z); strcpy(sep, ",");
        }
        f.printf("]}]");
        break;
      case f_2DRing:
        factor = 10; //in mm

        f.printf(",\"scale\":\"%s\"", "mm"); //scale currently not used (to show multiple fixtures)
        f.printf(",\"factor\":%d", factor);

        width = nrOfLeds/M_PI;
        height = nrOfLeds/M_PI;
        depth = 1;
        f.printf(",\"width\":%.0f", width);
        f.printf(",\"height\":%.0f", height);
        f.printf(",\"depth\":%.0f", depth);

        f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
        strcpy(sep, "");
        ringDiam = factor * nrOfLeds / 2 / M_PI; //in mm
        for (int i=0; i<nrOfLeds; i++) {
          float radians = i*360/nrOfLeds * (M_PI / 180);
          uint16_t x = factor * width / 2 + ringDiam * sinf(radians);
          uint16_t y = factor * height / 2 + ringDiam * cosf(radians);
          f.printf("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
        }
        f.printf("]}]");
        break;
      case f_2DRings241:
 
        factor = 10; //in mm

        f.printf(",\"scale\":\"%s\"", "mm"); //scale currently not used (to show multiple fixtures)
        f.printf(",\"factor\":%d", factor);

        width = 60/M_PI; //60 is outer ring
        height = 60/M_PI;
        depth = 1;
        f.printf(",\"width\":%.0f", width);
        f.printf(",\"height\":%.0f", height);
        f.printf(",\"depth\":%.0f", depth);

        f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
        strcpy(sep, "");

        in2out = mdl->getValue("in2out");

        // in2out or out2in
        arrSize = sizeof(rings)/sizeof(rings[0]);
        for (int j=in2out?0:arrSize-1; in2out?j<arrSize:j>=0; j+=in2out?1:-1) {
          uint8_t ringNrOfLeds = rings[j];
          ringDiam = factor * ringNrOfLeds / 2 / M_PI; //in mm
          for (int i=0; i<ringNrOfLeds; i++) {
            float radians = i*360/ringNrOfLeds * (M_PI / 180);
            uint16_t x = factor * width / 2 + ringDiam * sinf(radians);
            uint8_t y = factor * height / 2 + ringDiam * cosf(radians);
            f.printf("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
          }
        }
        f.printf("]}]");
        break;
      case f_2DMatrix:
        f.printf(",\"width\":%.0f", width);
        f.printf(",\"height\":%.0f", height);
        f.printf(",\"depth\":%.0f", depth);

        factor = 1;

        f.printf(",\"scale\":\"%s\"", "cm");
        f.printf(",\"factor\":%d", factor);

        f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
        strcpy(sep,"");
        for (uint8_t y = 0; y<height; y++)
          for (uint16_t x = 0; x<width ; x++) {
            // width = max(width, x);
            // height = max(height, y);
            // depth = 1;
            f.printf("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
          }
        f.printf("]}]");

        break;
      case f_3DCube:
        factor = 1;

        f.printf(",\"scale\":\"%s\"", "cm");
        f.printf(",\"factor\":%d", factor);

        width = 8;
        height = 8;
        depth = 8;
        f.printf(",\"width\":%.0f", width);
        f.printf(",\"height\":%.0f", height);
        f.printf(",\"depth\":%.0f", depth);

        f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
        strcpy(sep,"");
        for (uint8_t z = 0; z<depth; z++)
          for (uint8_t y = 0; y<height; y++)
            for (uint16_t x = 0; x<width ; x++) {
              f.printf("%s[%d,%d,%d]", sep, x,y,z); strcpy(sep, ",");
            }
        f.printf("]}]");

        break;
      case f_3DSideCube:
        factor = 1;

        f.printf(",\"scale\":\"%s\"", "dm");

        f.printf(",\"factor\":%d", factor);

        f.printf(",\"width\":%.0f", width);
        f.printf(",\"height\":%.0f", height);
        f.printf(",\"depth\":%.0f", depth);

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
        factor = 10;

        f.printf(",\"scale\":\"%s\"", "mm");
        f.printf(",\"factor\":%d", factor);

        width = 10;
        height = 10;
        depth = 10;
        f.printf(",\"width\":%.0f", width);
        f.printf(",\"height\":%.0f", height);
        f.printf(",\"depth\":%.0f", depth);

        f.printf(",\"leds\":[");
        strcpy(sep, "");
        ringDiam = factor * nrOfLeds / 2 / M_PI; //in mm
        for (int i=0; i<nrOfLeds; i++) {
          float radians = i*360/nrOfLeds * (M_PI / 180);
          uint16_t x = factor * width / 2 + ringDiam * sinf(radians);
          uint8_t y = factor * height / 2 + ringDiam * cosf(radians);
          uint8_t z = factor * height / 2 + ringDiam * cosf(radians);
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
  }

  static File openFile(const char * name) {
    char fileName[30] = "/";
    strcat(fileName, name);
    strcat(fileName, ".json");

    File f = files->open(fileName, "w");
    if (!f)
      print->print("ledFixGen Could not open file %s for writing\n", fileName);

    return f;
  }

};

static AppModLedFixGen *lfg;