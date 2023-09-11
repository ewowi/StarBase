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

  uint16_t distance = 1; //cm, not used yet (to display multiple fixture, also from other devices)

  uint8_t pinNr = 0;
  uint8_t pinList[9] = {255,255,255,255,255,255,255,255,255};
  uint8_t sizeOfPins = 0;

  char pinSep[2]="";
  char pixelSep[2]="";

  uint16_t width=0, height=0, depth=0, nrOfLeds=0;

  File f;

  GenFix() {
    print->print("GenFix construct\n");
    if (!mdl->getValue("pinList").isNull()) {
      print->print( "pinlist %s\n",mdl->getValue("pinList").as<const char *>());
      char str[32];
      strncpy(str, mdl->getValue("pinList").as<const char *>(), sizeof(str)-1);
      const char s[2] = ","; //delimiter
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
  }

  ~GenFix() {
    print->print("GenFix destruct\n");
  }

  void openHeader(const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    vsnprintf(name, sizeof(name)-1, format, args);

    va_end(args);

    f = files->open("/temp.json", "w");
    if (!f)
      print->print("GenFix could not open temp file for writing\n");

    f.print(",\"outputs\":[");
    strcpy(pinSep, "");
  }

  void closeHeader() {
    f.print("]"); //outputs

    print->print("closeHeader %d-%d-%d %d\n", width, height, depth, nrOfLeds);
    f.close();
    f = files->open("/temp.json", "r");

    File g;

    char fileName[32] = "/";
    strncat(fileName, name, sizeof(fileName)-1);
    strncat(fileName, ".json", sizeof(fileName)-1);


    //create g by merging in f (better solution?)
    g = files->open(fileName, "w");

    g.print("{");
    g.printf("\"name\":\"%s\"", name);
    g.printf(",\"nrOfLeds\":%d", nrOfLeds);
    g.printf(",\"width\":%d", width/10+1); //effects run on 1 led is 1 cm mode.
    g.printf(",\"height\":%d", height/10+1);
    g.printf(",\"depth\":%d", depth/10+1);

    byte character;
    f.read(&character, sizeof(byte));
    while (f.available()) {
      g.write(character);
      f.read(&character, sizeof(byte));
    }
    g.write(character);

    g.print("}");
    g.close();
    f.close();

    files->remove("/temp.json");

  }

  void openPin() {
    f.printf("%s{\"pin\":%d,\"leds\":[", pinSep, pinList[(pinNr++)%sizeOfPins]);
    strcpy(pinSep, ",");
    strcpy(pixelSep, "");
  }
  void closePin() {
    f.printf("]}");
  }

  void write2D(uint16_t x, uint16_t y) {
    f.printf("%s[%d,%d]", pixelSep, x, y);
    width = max(width, x);
    height = max(height, y);
    nrOfLeds++;
    strcpy(pixelSep, ",");
  }
  void write3D(uint16_t x, uint16_t y, uint16_t z) {
    f.printf("%s[%d,%d,%d]", pixelSep, x, y, z);
    width = max(width, x);
    height = max(height, y);
    depth = max(depth, z);
    nrOfLeds++;
    strcpy(pixelSep, ",");
  }

  void writef(const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char name[32];
    vsnprintf(name, sizeof(name)-1, format, args);

    va_end(args);

    f.print(name);          
    // print->print("GenFix printf %s\n", format);
  }

  void spiral1D (uint16_t startX, uint16_t startY, uint16_t startZ, uint16_t ledCount) {

    float width = 10;
    // float height = ledCount/12;
    float depth = 10;
    
    openPin();

    for (int i=0; i<ledCount; i++) {
      float radians = i*360/48 * (M_PI / 180); //48 leds per round
      uint16_t x = 10 * width/2 * (1 + sinf(radians));
      uint16_t y = 10 * i/12; //
      uint16_t z = 10 * depth/2 * (1+ cosf(radians));
      write3D(x + startX, y + startY, z + startZ);
    }

    closePin();
  }

  void matrix2D (uint16_t startX, uint16_t startY, uint16_t width, uint16_t height) {

    openPin();

    for (uint8_t y = 0; y<height; y++) //1cm distance between leds
      for (uint16_t x = 0; x<width ; x++) {
        write2D(x*10+startX,y*10+startY);
      }

    closePin();
  }

  void ring2D (uint16_t startX, uint16_t startY, uint16_t nrOfLeds) {

    // float size = nrOfLeds / 2 / M_PI;
    openPin();

    float ringDiam = 10 * nrOfLeds / 2 / M_PI; //in mm
    for (int i=0; i<nrOfLeds; i++) {
      float radians = i*360/nrOfLeds * (M_PI / 180);
      uint16_t x = ringDiam + ringDiam * sinf(radians);
      uint16_t y = ringDiam + ringDiam * cosf(radians);
      write2D(x+startX,y+startY);
    }
    closePin();
  }

  void wheel2D (uint16_t startX, uint16_t startY, uint16_t nrOfSpokes, uint16_t ledsPerSpoke) {

    float size = 50 + 10 * ledsPerSpoke;
    openPin();

    for (int i=0; i<nrOfSpokes; i++) {
      float radians = i*360/nrOfSpokes * (M_PI / 180);
      for (int j=0;j<ledsPerSpoke;j++) {
        float ringDiam = 50 + 10 * j; //in mm
        uint16_t x = size + ringDiam * sinf(radians);
        uint16_t y = size + ringDiam * cosf(radians);
        write2D(x+startX,y+startY);
      }
    }
    closePin();
  }

  void rings241 (uint16_t startX, uint16_t startY) {
    float ringDiam;
    uint8_t ringsNrOfLeds[9] = {1, 8, 12, 16, 24, 32, 40, 48, 60};
    uint8_t ringDiams[9] = {0, 13, 23, 33, 43, 53, 63, 73, 83}; //in mm
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

    openPin();

    in2out = mdl->getValue("in2out");

    // in2out or out2in
    uint16_t size = 60 / M_PI;
    for (int j=0; j<9; j++) {
      uint8_t ringNrOfLeds = in2out?ringsNrOfLeds[j]:ringsNrOfLeds[9 - 1 - j];
      ringDiam = in2out?ringDiams[j]:ringDiams[9 - 1 - j]; //in mm
      for (int i=0; i<ringNrOfLeds; i++) {
        float radians = i*360/ringNrOfLeds * (M_PI / 180);
        uint16_t x = 10 * size / 2 + ringDiam * sinf(radians);
        uint16_t y = 10 * size / 2 + ringDiam * cosf(radians);
        write2D(x + startX, y + startY);
      }
    }
    closePin();
  }

  void cloud (uint16_t startX, uint16_t startY) {
    //Small RL Alt Test

    uint8_t y;

    //first pin (red)
    openPin();
    y = 150; for (int x = 530; x >= 0; x-=10) write2D(x+startX,y+startY);
    y = 110; for (int x = 90; x <= 510; x+=10) write2D(x+startX,y+startY);
    y = 70; for (int x = 400; x >= 110; x-=10) write2D(x+startX,y+startY);
    closePin();
    //second pin (green)
    openPin();
    y = 140; for (int x = 530; x >= 0; x-=10) write2D(x+startX,y+startY);
    y = 100; for (int x = 90; x <= 510; x+=10) write2D(x+startX,y+startY);
    y = 60; for (int x = 390; x >= 120; x-=10) write2D(x+startX,y+startY);
    closePin();
    //third pin (blue)
    openPin();
    y = 130; for (int x = 520; x >= 10; x-=10) write2D(x+startX,y+startY);
    y = 90; for (int x = 100; x <= 500; x+=10) write2D(x+startX,y+startY);
    y = 50; for (int x = 390; x >= 140; x-=10) write2D(x+startX,y+startY);
    closePin();
    //fourth pin (yellow)
    openPin();
    y = 120; for (int x = 520; x >= 30; x-=10) write2D(x+startX,y+startY);
    y = 80; for (int x = 100; x <= 480; x+=10) write2D(x+startX,y+startY);
    y = 40; for (int x = 380; x >= 240; x-=10) write2D(x+startX,y+startY);
    y = 30; for (int x = 250; x <= 370; x+=10) write2D(x+startX,y+startY);
    y = 20; for (int x = 360; x >= 260; x-=10) write2D(x+startX,y+startY);
    y = 10; for (int x = 270; x <= 350; x+=10) write2D(x+startX,y+startY);
    y = 00; for (int x = 330; x >= 290; x-=10) write2D(x+startX,y+startY);
    closePin();
  }

  void cone3D (uint16_t startX, uint16_t startY, uint16_t startZ, uint8_t nrOfRings) {

    openPin();

    float width = nrOfRings*3/M_PI;
    float height = nrOfRings;
    // float depth = nrOfRings*3/M_PI;
    // , nrOfLeds

    bool in2out = mdl->getValue("in2out");

    for (int j=0; j<nrOfRings; j++) {
      uint8_t ringNrOfLeds = (j+1) * 3;
      float ringDiam = 10*ringNrOfLeds / 2 / M_PI; //in mm
      for (int i=0; i<ringNrOfLeds; i++) {
        float radians = i*360/ringNrOfLeds * (M_PI / 180);
        uint16_t x = 10* width / 2 + ringDiam * sinf(radians);
        uint16_t z = 10 * height / 2 + ringDiam * cosf(radians);
        uint16_t y = j*10;
        write3D(x + startX, y + startY, z + startZ);
      }
    }

    closePin();
  }

  void sideCube3D (uint16_t startX, uint16_t startY, uint16_t startZ, uint16_t width, uint8_t sides) {
    //front and back
    for (uint8_t z = 0; z<width; z+=width-1) {
      openPin();
      for (uint8_t y = 0; y<width; y++)
        for (uint16_t x = 0; x<width ; x++) {
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
      closePin();
    }
    //NO botom and top
    for (uint8_t y = width-1; y<width; y+=width-1) {
      openPin();
      for (uint8_t z = 0; z<width; z++)
        for (uint16_t x = 0; x<width ; x++) {
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
      closePin();
    }

    //left and right
    for (uint16_t x = 0; x<width ; x+=width-1) {
      openPin();
      for (uint8_t z = 0; z<width; z++)
        for (uint8_t y = 0; y<width; y++) {
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
      closePin();
    }
  }

  void cube3D (uint16_t startX, uint16_t startY, uint16_t startZ, uint16_t width, uint16_t height, uint16_t depth) {
      openPin();

      for (uint8_t z = 0; z<depth; z++)
        for (uint8_t y = 0; y<height; y++)
          for (uint16_t x = 0; x<width ; x++) {
            write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
          }
        
      closePin();
  }

  void globe3D (uint16_t startX, uint16_t startY, uint16_t startZ, uint16_t width) {
      openPin();

      float ringDiam = 10 * width / 2 / M_PI; //in mm
      for (int i=0; i<width; i++) {
        float radians = i*360/width * (M_PI / 180);
        uint16_t x = 10 * width/M_PI / 2 + ringDiam * sinf(radians);
        uint16_t y = 10 * width / 2 + ringDiam * cosf(radians);
        uint16_t z = 10 * width / 2 + ringDiam * cosf(radians);
        write3D(x + startX, y + startY, z + startZ);
      }

      closePin();
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
      select.add("2DWall"); //5
      select.add("2DWheel"); //6
      select.add("3DCone"); //7
      select.add("3DSideCube"); //8
      select.add("3DCube"); //9
      select.add("3DGlobe"); //10
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
    f_2DWall,
    f_2DWheel,
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
    else if (value == f_2DWheel) {
      ui->initNumber(parentVar, "nrOfSpokes", 36, false);
      ui->initNumber(parentVar, "ledsPerSpoke", 24, false);
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

    GenFix genFix;

    if (fix == f_1DSpiral) {

      uint16_t ledCount = mdl->getValue("ledCount");

      genFix.openHeader("1DSpiral%02d", ledCount);

      genFix.spiral1D(0, 0, 0, ledCount);

      genFix.closeHeader();
      
    } else if (fix == f_2DMatrix) {
      uint16_t width = mdl->getValue("width");
      uint16_t height = mdl->getValue("height");

      genFix.openHeader("2DMatrix%02d%02d", width, height);

      genFix.matrix2D(0, 0, width, height);

      genFix.closeHeader();

    } else if (fix == f_2DRing) {
      uint16_t ledCount = mdl->getValue("ledCount");

      genFix.openHeader("2DRing%02d", ledCount);

      genFix.ring2D(0, 0, ledCount);

      genFix.closeHeader();
    } else if (fix == f_2DRings241) {

      genFix.openHeader("2DRing241");

      genFix.rings241(0, 0);

      genFix.closeHeader();

    } else if (fix == f_2DCloud) {

      genFix.openHeader("2DCloud5416");

      genFix.cloud(0, 0);

      genFix.closeHeader();
    }
    else if (fix == f_2DWall) {

      genFix.openHeader("2DWall");

      genFix.rings241(0, 0);

      genFix.matrix2D(190, 0, 8, 8);

      genFix.matrix2D(0, 190, 50, 4);

      genFix.ring2D(190, 85, 48);

      // genFix.spiral1D(240, 0, 0, 48);

      genFix.closeHeader();
    }
    else if (fix == f_2DWheel) {
      uint16_t nrOfSpokes = mdl->getValue("nrOfSpokes");
      uint16_t ledsPerSpoke = mdl->getValue("ledsPerSpoke");

      genFix.openHeader("2DWheel%02d_%d", nrOfSpokes, ledsPerSpoke);

      genFix.wheel2D(0, 0, nrOfSpokes, ledsPerSpoke);

      genFix.closeHeader();

    } else if (fix == f_3DCone) {

      //calculate nrOfLeds
      uint8_t nrOfRings = mdl->getValue("nrOfRings");
      // uint16_t nrOfLeds = 0;
      // for (int j=0; j<nrOfRings; j++) {
      //   nrOfLeds += (j+1) * 3;
      // }

      genFix.openHeader("3DCone%02d", nrOfRings);

      genFix.cone3D(0,0,0, nrOfRings);

      genFix.closeHeader();
    }
    else if (fix == f_3DSideCube) {
      uint16_t width = mdl->getValue("width");
      uint8_t sides = mdl->getValue("sides");
      
      genFix.openHeader("3DSideCube%02d%02d%02d", width, width, sides);

      genFix.sideCube3D (0, 0, 0, width, sides);  

      genFix.closeHeader();
    } else if (fix == f_3DCube) {
      uint16_t width = mdl->getValue("width");
      uint16_t height =  mdl->getValue("height");
      uint16_t depth = mdl->getValue("depth");

      genFix.openHeader("3DCube%02d%02d%02d", width, height, depth);

      genFix.cube3D(0, 0, 0, width, height, depth);

      genFix.closeHeader();

    } else if (fix == f_3DGlobe) {

      uint16_t width = mdl->getValue("width");

      genFix.openHeader("3DGlobe%02d", width);

      genFix.globe3D(0, 0, 0, width);

      genFix.closeHeader();
    }

    files->filesChange();

    //reload ledfix select
    ui->processUiFun("ledFix");
  }

  static File openFile(const char * name) {
    char fileName[30] = "/";
    strncat(fileName, name, sizeof(fileName)-1);
    strncat(fileName, ".json", sizeof(fileName)-1);

    File f = files->open(fileName, "w");
    if (!f)
      print->print("ledFixGen Could not open file %s for writing\n", fileName);

    return f;
  }

};

static AppModLedFixGen *lfg;