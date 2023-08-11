/*
   @title     StarMod
   @file      AppModLedFixGen.h
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "Module.h"

class GenFix {

public:
  char name[32] = "";

  float width = 8; //float to deal with rings, globes (sin, cos stuff)
  float height = 8;
  float depth = 1;
  uint16_t nrOfLeds = 64; //default
  uint16_t factor = 1;
  uint16_t distance = 1; //cm, not used yet (to display multiple fixture, also from other devices)

  File f;

  GenFix() {
    print->print("GenFix construct\n");
  }

  ~GenFix() {
    print->print("GenFix destruct\n");
    f.print("}");
    f.close();
  }

  void assign(float width, float height, float depth, uint16_t nrOfLeds, uint16_t factor) {
    this->width = width;
    this->height = height;
    this->depth = depth;
    this->nrOfLeds = nrOfLeds;
    this->factor = factor;
  }

  void writeName(const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    vsnprintf(name, sizeof(name), format, args);

    va_end(args);

    char fileName[32] = "/";
    strcat(fileName, name);
    strcat(fileName, ".json");

    f = files->open(fileName, "w");
    if (!f)
      print->print("GenFix could not open file %s for writing\n", fileName);
    else
      print->print("GenFix writeName %s\n", fileName);
  }

  void writeHeader() {
    f.print("{");

    f.printf("\"name\":\"%s\"", name);
    f.printf(",\"nrOfLeds\":%d", nrOfLeds);
    // f.printf(",\"distance\":\"%d\"", distance);
    f.printf(",\"factor\":%d", factor);

    f.printf(",\"width\":%.0f", width);
    f.printf(",\"height\":%.0f", height);
    f.printf(",\"depth\":%.0f", depth);
  }

  void writef(const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char name[32];
    vsnprintf(name, sizeof(name), format, args);

    va_end(args);

    f.print(name);          
    // print->print("GenFix printf %s\n", format);
  }

};

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
      select.add("2DCloud"); //4
      select.add("3DCone"); //5
      select.add("3DSideCube"); //6
      select.add("3DCube"); //7
      select.add("3DGlobe"); //8
    }, [](JsonObject var) { //chFun
      ledFixGenChFun(var);
    }); //ledFixGen

    ui->initText(parentVar, "pinList", "16", false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "One or more e.g. 12,13");
    });

    ui->initButton(parentVar, "generate", nullptr, false, [](JsonObject var) { //uiFun
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
    f_2DCloud,
    f_3DCone,
    f_3DSideCube,
    f_3DCube,
    f_3DGlobe,
    count
  };

  //generate dynamic html for fixture controls
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
    else if (value == f_3DCone) {
      ui->initCheckBox(parentVar, "in2out", true, false);
      ui->initNumber(parentVar, "nrOfRings", 24, false);
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
      ui->initNumber(parentVar, "width", 24, false);
    }

    web->sendDataWs(parentVar); //always send, also when no children, to remove them from ui
  }

  static void generateChFun(JsonObject var) {

    uint8_t fix = mdl->getValue("ledFixGen");

    char sep[3]="";
    char sep2[3]="";

    uint8_t pinList[9] = {255,255,255,255,255,255,255,255,255};
    uint8_t sizeOfPins = 0;
    if (!mdl->getValue("pinList").isNull()) {
      print->print( "pinlist %s\n",mdl->getValue("pinList").as<const char *>());
      char str[32];
      strcpy(str, mdl->getValue("pinList").as<const char *>());
      const char s[2] = ",";
      char *token;
      /* get the first token */
      token = strtok(str, s);
      /* walk through other tokens */
      while( token != NULL ) 
      {
        print->print( " %s(%d) %d\n", token, atoi(token), sizeOfPins );
        pinList[sizeOfPins++] = atoi(token);
        token = strtok(NULL, s);
      }
    }

    uint8_t pinNr = 0;
    float ringDiam;

    GenFix genFix;
    
    if (fix == f_1DSpiral) {

      uint16_t ledCount = mdl->getValue("ledCount");
      genFix.assign(10, ledCount/12, 10, ledCount, 10);

      genFix.writeName("1DSpiral%02d", ledCount);

      genFix.writeHeader();

      genFix.writef(",\"outputs\":[{\"pin\":%d,\"leds\":[", pinList[(pinNr++)%sizeOfPins]);
      strcpy(sep, "");
      for (int i=0; i<genFix.nrOfLeds; i++) {
        float radians = i*360/48 * (M_PI / 180); //48 leds per round
        uint16_t x = genFix.factor * genFix.width/2 * (1 + sinf(radians));
        uint16_t y = genFix.factor * i/12; //
        uint16_t z = genFix.factor * genFix.depth/2 * (1+ cosf(radians));
        genFix.writef("%s[%d,%d,%d]", sep, x, y, z); strcpy(sep, ",");
      }
      genFix.writef("]}]");
    } else if (fix == f_2DMatrix) {
      uint16_t width = mdl->getValue("width");
      uint16_t height = mdl->getValue("height");
      genFix.assign(width, height, 1, width * height, 1);

      genFix.writeName("2DMatrix%02d%02d", width, height);
      genFix.writeHeader();

      genFix.writef(",\"outputs\":[{\"pin\":%d,\"leds\":[", pinList[(pinNr++)%sizeOfPins]);
      strcpy(sep,"");
      for (uint8_t y = 0; y<height; y++)
        for (uint16_t x = 0; x<width ; x++) {
          genFix.writef("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
        }
      genFix.writef("]}]");
    } else if (fix == f_2DRing) {
      uint16_t ledCount = mdl->getValue("ledCount");
      genFix.assign(ledCount/M_PI, ledCount/M_PI, 1, ledCount, 10);

      genFix.writeName("2DRing%02d", ledCount);
      genFix.writeHeader();

      genFix.writef(",\"outputs\":[{\"pin\":%d,\"leds\":[", pinList[(pinNr++)%sizeOfPins]);
      strcpy(sep, "");
      ringDiam = genFix.factor * genFix.nrOfLeds / 2 / M_PI; //in mm
      for (int i=0; i<genFix.nrOfLeds; i++) {
        float radians = i*360/genFix.nrOfLeds * (M_PI / 180);
        uint16_t x = genFix.factor * genFix.width / 2 + ringDiam * sinf(radians);
        uint16_t y = genFix.factor * genFix.height / 2 + ringDiam * cosf(radians);
        genFix.writef("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
      }
      genFix.writef("]}]");
    } else if (fix == f_2DRings241) {

      uint8_t ringsNrOfLeds[9] = {1, 8, 12, 16, 24, 32, 40, 48, 60};
      uint8_t ringDiams[9] = {0, 13, 23, 33, 43, 53, 63, 73, 83};
      //  {0, 0},     //0 Center Point -> 1
      //   {1, 8},     //1 -> 8
      //   {9, 20},   //2 -> 12
      //   {21, 36},   //3 -> 16
      //   {37, 60},   //4 -> 24
      //   {61, 92},   //5 -> 32
      //   {93, 132},  //6 -> 40
      //   {133, 180}, //7 -> 48
      //   {181, 240}, //8 Outer Ring -> 60   -> 241

      bool in2out;

      genFix.assign(60/M_PI, 60/M_PI, 1, 241, 10);

      genFix.writeName("2DRing%02d", genFix.nrOfLeds);
      genFix.writeHeader();

      genFix.writef(",\"outputs\":[{\"pin\":%d,\"leds\":[", pinList[(pinNr++)%sizeOfPins]);
      strcpy(sep, "");

      in2out = mdl->getValue("in2out");

      // in2out or out2in
      int arrSize = sizeof(ringsNrOfLeds)/sizeof(ringsNrOfLeds[0]);
      for (int j=0; j<arrSize; j++) {
        uint8_t ringNrOfLeds = in2out?ringsNrOfLeds[j]:ringsNrOfLeds[arrSize - 1 - j];
        ringDiam = in2out?ringDiams[j]:ringDiams[arrSize - 1 - j]; //in mm
        for (int i=0; i<ringNrOfLeds; i++) {
          float radians = i*360/ringNrOfLeds * (M_PI / 180);
          uint16_t x = genFix.factor * genFix.width / 2 + ringDiam * sinf(radians);
          uint16_t y = genFix.factor * genFix.height / 2 + ringDiam * cosf(radians);
          genFix.writef("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
        }
      }
      genFix.writef("]}]");
    } else if (fix == f_2DCloud) {
      uint16_t width = 54;
      uint16_t height = 16;
      genFix.assign(width, height, 1, 513, 1);

      genFix.writeName("2DCloud%02d%02d", width, height);
      genFix.writeHeader();

      genFix.writef(",\"outputs\":[");

      //Small RL Alt Test

      uint8_t y;
      strcpy(sep,"");

      //first pin (red)
      genFix.writef("%s{\"pin\":%d,\"leds\":[", sep, pinList[(pinNr++)%sizeOfPins]);strcpy(sep, ",");
      strcpy(sep2,"");
      y = 15; for (int x = 53; x >= 0; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 11; for (int x = 9; x <= 51; x++) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 7; for (int x = 40; x >= 11; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      genFix.writef("]}");
      //second pin (green)
      genFix.writef("%s{\"pin\":%d,\"leds\":[", sep, pinList[(pinNr++)%sizeOfPins]);strcpy(sep, ",");
      strcpy(sep2,"");
      y = 14; for (int x = 53; x >= 0; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 10; for (int x = 9; x <= 51; x++) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 6; for (int x = 39; x >= 12; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      genFix.writef("]}");
      //third pin (blue)
      genFix.writef("%s{\"pin\":%d,\"leds\":[", sep, pinList[(pinNr++)%sizeOfPins]);strcpy(sep, ",");
      strcpy(sep2,"");
      y = 13; for (int x = 52; x >= 1; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 9; for (int x = 10; x <= 50; x++) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 5; for (int x = 39; x >= 14; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      genFix.writef("]}");
      //fourth pin (yellow)
      genFix.writef("%s{\"pin\":%d,\"leds\":[", sep, pinList[(pinNr++)%sizeOfPins]);strcpy(sep, ",");
      strcpy(sep2,"");
      y = 12; for (int x = 52; x >= 3; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 8; for (int x = 10; x <= 48; x++) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 4; for (int x = 38; x >= 24; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 3; for (int x = 25; x <= 37; x++) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 2; for (int x = 36; x >= 26; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 1; for (int x = 27; x <= 35; x++) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      y = 0; for (int x = 33; x >= 29; x--) {genFix.writef("%s[%d,%d]", sep2, x,y); strcpy(sep2, ",");}
      genFix.writef("]}");

      genFix.writef("]");
    } else if (fix == f_3DCone) {

      bool in2out;

      uint8_t nrOfRings = mdl->getValue("nrOfRings");
      uint16_t nrOfLeds = 0;
      for (int j=0; j<nrOfRings; j++) {
        nrOfLeds += (j+1) * 3;
      }
      genFix.assign(nrOfRings*3/M_PI, nrOfRings, nrOfRings*3/M_PI, nrOfLeds, 10);

      genFix.writeName("3DCone%02d", nrOfRings);
      genFix.writeHeader();

      genFix.writef(",\"outputs\":[{\"pin\":%d,\"leds\":[", pinList[(pinNr++)%sizeOfPins]);
      strcpy(sep, "");

      in2out = mdl->getValue("in2out");


      for (int j=0; j<nrOfRings; j++) {
        uint8_t ringNrOfLeds = (j+1) * 3;
        ringDiam = genFix.factor * ringNrOfLeds / 2 / M_PI; //in mm
        for (int i=0; i<ringNrOfLeds; i++) {
          float radians = i*360/ringNrOfLeds * (M_PI / 180);
          uint16_t x = genFix.factor * genFix.width / 2 + ringDiam * sinf(radians);
          uint16_t z = genFix.factor * genFix.height / 2 + ringDiam * cosf(radians);
          uint16_t y = j*genFix.factor;
          genFix.writef("%s[%d,%d,%d]", sep, x,y,z); strcpy(sep, ",");
        }
      }
      genFix.writef("]}]");
    }
    else if (fix == f_3DSideCube) {
      uint16_t width = mdl->getValue("width");
      uint8_t sides = mdl->getValue("sides");
      genFix.assign(width, width, width, width*width*sides, 1);

      genFix.writeName("3DSideCube%02d%02d%02d", width, width, sides);
      genFix.writeHeader();

      genFix.writef(",\"outputs\":[");
  
      strcpy(sep,"");
      strcpy(sep2,"");

      //front and back
      for (uint8_t z = 0; z<genFix.depth; z+=genFix.depth-1) {
        genFix.writef("%s{\"pin\":%d,\"leds\":[", sep, pinList[(pinNr++)%sizeOfPins]);strcpy(sep, ",");
        strcpy(sep2,"");
        for (uint8_t y = 0; y<genFix.height; y++)
          for (uint16_t x = 0; x<genFix.width ; x++) {
            genFix.writef("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
          }
        genFix.writef("]}");
      }
      //NO botom and top
      for (uint8_t y = genFix.height-1; y<genFix.height; y+=genFix.height-1) {
        genFix.writef("%s{\"pin\":%d,\"leds\":[", sep, pinList[(pinNr++)%sizeOfPins]);strcpy(sep, ",");
        strcpy(sep2,"");
        for (uint8_t z = 0; z<genFix.depth; z++)
          for (uint16_t x = 0; x<genFix.width ; x++) {
            genFix.writef("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
          }
        genFix.writef("]}");
      }

      //left and right
      for (uint16_t x = 0; x<genFix.width ; x+=genFix.width-1) {
        genFix.writef("%s{\"pin\":%d,\"leds\":[", sep, pinList[(pinNr++)%sizeOfPins]);strcpy(sep, ",");
        strcpy(sep2,"");
        for (uint8_t z = 0; z<genFix.depth; z++)
          for (uint8_t y = 0; y<genFix.height; y++) {
            genFix.writef("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
          }
        genFix.writef("]}");
      }
    
      genFix.writef("]");
    } else if (fix == f_3DCube) {
      uint16_t width = mdl->getValue("width");
      uint16_t height =  mdl->getValue("height");
      uint16_t depth = mdl->getValue("depth");
      genFix.assign(width, height, depth, width*height*depth, 1);

      genFix.writeName("3DCube%02d%02d%02d", width, height, depth);
      genFix.writeHeader();

      genFix.writef(",\"outputs\":[{\"pin\":%d,\"leds\":[", pinList[(pinNr++)%sizeOfPins]);
      strcpy(sep,"");
      for (uint8_t z = 0; z<genFix.depth; z++)
        for (uint8_t y = 0; y<genFix.height; y++)
          for (uint16_t x = 0; x<genFix.width ; x++) {
            genFix.writef("%s[%d,%d,%d]", sep, x,y,z); strcpy(sep, ",");
          }
      genFix.writef("]}]");
    } else if (fix == f_3DGlobe) {

      uint16_t width = mdl->getValue("width");
      genFix.assign(width/M_PI, width, width, width, 10);

      genFix.writeName("3DGlobe%02d", width);

      genFix.writeHeader();

      genFix.writef(",\"outputs\":[{\"pin\":%d,\"leds\":[", pinList[(pinNr++)%sizeOfPins]);
      strcpy(sep, "");
      ringDiam = genFix.factor * genFix.nrOfLeds / 2 / M_PI; //in mm
      for (int i=0; i<genFix.nrOfLeds; i++) {
        float radians = i*360/genFix.nrOfLeds * (M_PI / 180);
        uint16_t x = genFix.factor * genFix.width / 2 + ringDiam * sinf(radians);
        uint16_t y = genFix.factor * genFix.height / 2 + ringDiam * cosf(radians);
        uint16_t z = genFix.factor * genFix.height / 2 + ringDiam * cosf(radians);
        genFix.writef("%s[%d,%d,%d]", sep, x,y,z); strcpy(sep, ",");
      }
      genFix.writef("]}]");
    }

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