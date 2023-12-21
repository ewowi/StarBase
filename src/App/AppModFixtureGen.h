/*
   @title     StarMod
   @file      AppModFixtureGen.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "SysModule.h"

class GenFix {

public:
  char name[32] = "";

  uint16_t distance = 1; //cm, not used yet (to display multiple fixture, also from other devices)

  uint8_t pinNr = 0;
  uint8_t pinList[9] = {255,255,255,255,255,255,255,255,255};
  uint8_t nrOfPins = 0;

  char pinSep[2]="";
  char pixelSep[2]="";

  uint16_t width=0, height=0, depth=0, nrOfLeds=0;

  File f;

  GenFix() {
    USER_PRINTF("GenFix construct\n");
    if (!mdl->getValue("pinList").isNull()) {
      USER_PRINTF( "pinlist %s\n",mdl->getValue("pinList").as<const char *>());
      char str[32];
      strncpy(str, mdl->getValue("pinList").as<const char *>(), sizeof(str)-1);
      const char s[2] = ","; //delimiter
      char *token;
      /* get the first token */
      token = strtok(str, s);
      /* walk through other tokens */
      while( token != NULL ) 
      {
        USER_PRINTF( " %s(%d) %d\n", token, atoi(token), nrOfPins );
        pinList[nrOfPins++] = atoi(token);
        token = strtok(NULL, s);
      }
    }
  }

  ~GenFix() {
    USER_PRINTF("GenFix destruct\n");
  }

  void openHeader(const char * format, ...) {
    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    vsnprintf(name, sizeof(name)-1, format, args);

    va_end(args);

    f = files->open("/temp.json", "w");
    if (!f)
      USER_PRINTF("GenFix could not open temp file for writing\n");

    f.print(",\"outputs\":[");
    strcpy(pinSep, "");
  }

  void closeHeader() {
    f.print("]"); //outputs

    USER_PRINTF("closeHeader %d-%d-%d %d\n", width, height, depth, nrOfLeds);
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

  void 
  openPin() {
    uint8_t nextPin;
    if (pinNr < nrOfPins)
      nextPin = pinNr++;
    else
      nextPin = nrOfPins-1;
    f.printf("%s{\"pin\":%d,\"leds\":[", pinSep, pinList[nextPin]);
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
    // USER_PRINTF("GenFix printf %s\n", format);
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

    //qad setup of serpentine, should be done better!
    bool serpentine = mdl->getValue("serpentine");

    if (serpentine) {
      for (uint8_t y = 0; y<height; y++) { //1cm distance between leds
        if (y%2==0)
          for (uint16_t x = 0; x<width ; x++) {
            write2D(x*10+startX,y*10+startY);
          }
        else
          for (int x = width-1; x>=0 ; x--) {
            write2D(x*10+startX,y*10+startY);
          }
      }
    }
    else {
      for (uint8_t y = 0; y<height; y++) //1cm distance between leds
        for (uint16_t x = 0; x<width ; x++) {
          write2D(x*10+startX,y*10+startY);
        }
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

  void plane3DFindNextPoint(Coordinate *point, Coordinate first, Coordinate last, uint8_t axis, bool clockWise) {
    if (axis == 0) {
      if (point->x != last.x) {
        point->x += point->x<=last.x?1:-1;
      }
      else { //cr
        point->x = first.x;
        if (clockWise) plane3DFindNextPoint(point, first, last, 1, clockWise);
        else plane3DFindNextPoint(point, first, last, 2, clockWise);
      }
    }
    if (axis == 1) {
      if (point->y != last.y) {
        point->y += point->y<=last.y?1:-1;
      }
      else { //cr
        point->y = first.y;
        if (clockWise) plane3DFindNextPoint(point, first, last, 2, clockWise);
        else plane3DFindNextPoint(point, first, last, 0, clockWise);
      }
    }
    if (axis == 2) {
      if (point->z != last.z) {
        point->z += point->z<=last.z?1:-1;
      }
      else { //cr
        point->z = first.z;
        if (clockWise) plane3DFindNextPoint(point, first, last, 0, clockWise);
        else plane3DFindNextPoint(point, first, last, 1, clockWise);
      }
    }

    // USER_PRINTF("plane3DFindNextPoint %d %d %d %d \n", axis, point->x, point->y, point->z);
  }

  void plane3D (Coordinate first, Coordinate last, bool clockWise) {
    openPin();
    bool cont = true;
    Coordinate point = first;

    write3D(point.x*10, point.y*10, point.z*10);

    while (cont) {

      plane3DFindNextPoint(&point, first, last, 0, clockWise);

      write3D(point.x*10, point.y*10, point.z*10);

      //check if next point is end point
      cont = (point.x != last.x || point.y != last.y || point.z != last.z);
      // USER_PRINTF("plane3DFindNextPoint p:%d %d %d %d l:%d %d %d \n", cont, point.x, point.y, point.z, last.x, last.y, last.z);
    }

    closePin();
  }

  //deprecated (use plane3D)
  void sideCube3D (uint16_t startX, uint16_t startY, uint16_t startZ, uint16_t length, uint8_t sides) {
    //front
    { //for (uint8_t z = 0; z<length; z+=length-1) 
      int z = 0;
      openPin();
      for (int y = 0; y<length; y++)
        for (int x = 0; x<length ; x++) {
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
      closePin();
    }
    //right
    { //for (uint16_t x = 0; x<length ; x+=length-1) 
      int x = length-1;
      openPin();
      for (int y = 0; y<length; y++) {
        for (int z = 0; z<length; z++)
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
      closePin();
    }
    //back
    { //for (uint8_t z = 0; z<length; z+=length-1) 
      int z = length-1;
      openPin();
      for (int y = 0; y<length; y++)
        for (int x = length-1; x>=0 ; x--) {
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
      closePin();
    }
    //bottom
    { //for (uint8_t y = length-1; y<length; y+=length-1) 
      uint16_t y = length -1;
      openPin();
      for (int z = length-1; z>=0; z--)
        for (int x = length-1; x>=0 ; x--) {
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
      closePin();
    }
    //left
    { //for (uint16_t x = 0; x<length ; x+=length-1) 
      int x = 0;
      openPin();
      for (int y = length-1; y>=0; y--) {
        for (int z = length-1; z>=0; z--) {
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
      }
      closePin();
    }
    //top
    { //for (uint16_t x = 0; x<length ; x+=length-1) 
      int y = 0;
      openPin();
      for (int z = length-1; z>=0; z--) {
        for (int x = 0; x<length; x++) {
          write3D(x*10 + startX, y*10 + startY, z*10 + startZ);
        }
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

  // https://stackoverflow.com/questions/17705621/algorithm-for-a-geodesic-sphere
  //https://opengl.org.ru/docs/pg/0208.html
  void geodesicDome3D (uint16_t startX, uint16_t startY, uint16_t startZ) {
 
    uint8_t tindices[20][3] = {    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},       {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},       {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };

    openPin();

    for (int i=0; i<20; i++) {
      write3D(tindices[i][0]*10 + startX, tindices[i][1]*10 + startY, tindices[i][2]*10 + startZ);
    }
    closePin();
  }

};

class AppModFixtureGen:public SysModule {

public:

  AppModFixtureGen() :SysModule("Fixture Generator") {};

  void setup() {
    SysModule::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    parentVar = ui->initModule(parentVar, name);

    ui->initSelect(parentVar, "fixtureGen", 0, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Fixture");
      web->addResponse(var["id"], "comment", "Type of fixture");
      JsonArray select = web->addResponseA(var["id"], "data");
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
      select.add("3DGlobe WIP"); //10
      select.add("3DGeodesicDome WIP"); //11
    }, [this](JsonObject var, uint8_t) { //chFun
      fixtureGenChFun(var);
    }); //fixtureGen

    ui->initText(parentVar, "pinList", "16", 32, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "One or more e.g. 12,13,14");
    });

    ui->initButton(parentVar, "generate", nullptr, false, [](JsonObject var) { //uiFun
    }, [this](JsonObject var, uint8_t) { //chFun
      generateChFun(var);
    });

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // SysModule::loop();
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
    f_3DGeodesicDome,
    count
  };

  //generate dynamic html for fixture controls
  void fixtureGenChFun(JsonObject var) {
    JsonObject parentVar = mdl->findVar(var["id"]);
    parentVar.remove("n"); //tbd: we should also remove the uiFun and chFun !!
    uint8_t value = var["value"];
    
    if (value == f_1DSpiral) {
      ui->initNumber(parentVar, "ledCount", 64, 1, NUM_LEDS_Preview);
    }
    else if (value == f_2DRing) {
      ui->initNumber(parentVar, "ledCount", 24, 1, NUM_LEDS_Preview);
    }
    else if (value == f_2DRings241) {
      ui->initCheckBox(parentVar, "in2out", true);
    }
    else if (value == f_2DWheel) {
      ui->initNumber(parentVar, "nrOfSpokes", 36, 1, 360);
      ui->initNumber(parentVar, "ledsPerSpoke", 24, 1, 360);
    }
    else if (value == f_3DCone) {
      ui->initCheckBox(parentVar, "in2out", true);
      ui->initNumber(parentVar, "nrOfRings", 24, 1, 360);
    }
    else if (value == f_2DMatrix) {
      ui->initNumber(parentVar, "width", 8, 1, 255);

      ui->initNumber(parentVar, "height", 8, 1, 255);

      ui->initSelect(parentVar, "firstLedX", 0, false, [](JsonObject var) { //uiFun
        // web->addResponse(var["id"], "label", "fixture generator");
        JsonArray select = web->addResponseA(var["id"], "data");
        select.add("Left"); //0
        select.add("Right"); //1
      });
      ui->initSelect(parentVar, "firstLedY", 0, false, [](JsonObject var) { //uiFun
        // web->addResponse(var["id"], "label", "fixture generator");
        JsonArray select = web->addResponseA(var["id"], "data");
        select.add("Top"); //0
        select.add("Bottom"); //1
      });

      ui->initCheckBox(parentVar, "serpentine");
    }
    else if (value == f_3DCube) {
      ui->initNumber(parentVar, "width", 8, 1, 16);
      ui->initNumber(parentVar, "height", 8, 1, 16);
      ui->initNumber(parentVar, "depth", 8, 1, 16);
    }
    else if (value == f_3DSideCube) {
      ui->initNumber(parentVar, "length", 8, 1, 32);
      ui->initNumber(parentVar, "sides", 5, 1, 6);
    }
    else if (value == f_3DGlobe) {
      ui->initNumber(parentVar, "width", 24, 1, 16);
    }

    JsonDocument *responseDoc = web->getResponseDoc();
    responseDoc->clear(); //needed for deserializeJson?
    JsonObject responseObject = responseDoc->to<JsonObject>();

    responseObject["details"] = parentVar;

    print->printJson("parentVar", responseObject);
    web->sendDataWs(responseObject); //always send, also when no children, to remove them from ui

  }

  void generateChFun(JsonObject var) {

    uint8_t fix = mdl->getValue("fixtureGen");

    GenFix genFix;

    if (fix == f_1DSpiral) {

      uint16_t ledCount = mdl->getValue("ledCount");

      genFix.openHeader("1DSpiral%d", ledCount);

      genFix.spiral1D(0, 0, 0, ledCount);

      genFix.closeHeader();
      
    } else if (fix == f_2DMatrix) {
      uint16_t width = mdl->getValue("width");
      uint16_t height = mdl->getValue("height");

      genFix.openHeader("2DMatrix%d%d", width, height);

      genFix.matrix2D(0, 0, width, height);

      genFix.closeHeader();

    } else if (fix == f_2DRing) {
      uint16_t ledCount = mdl->getValue("ledCount");

      genFix.openHeader("2DRing%d", ledCount);

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

      genFix.openHeader("2DWheel%d_%d", nrOfSpokes, ledsPerSpoke);

      genFix.wheel2D(0, 0, nrOfSpokes, ledsPerSpoke);

      genFix.closeHeader();

    } else if (fix == f_3DCone) {

      //calculate nrOfLeds
      uint8_t nrOfRings = mdl->getValue("nrOfRings");
      // uint16_t nrOfLeds = 0;
      // for (int j=0; j<nrOfRings; j++) {
      //   nrOfLeds += (j+1) * 3;
      // }

      genFix.openHeader("3DCone%d", nrOfRings);

      genFix.cone3D(0,0,0, nrOfRings);

      genFix.closeHeader();
    }
    else if (fix == f_3DSideCube) {
      uint16_t length = mdl->getValue("length");
      uint8_t sides = mdl->getValue("sides");
      
      genFix.openHeader("3DSideCube%d%d", length, sides);

      genFix.plane3D({1, 1, 0},   {length, length, 0}, true); // front (z=0, first x then y)
      genFix.plane3D({length, length+1, length}, {1, length+1, 1}, false); // bottom (y=length+1, first x, then z)
      genFix.plane3D({length+1, 1, 1}, {length+1, length, length}, false); // right (x=length + 1, first z, then y)
      genFix.plane3D({0, length, length}, {0, 1, 1}, true); // left (x=0, first y, then z)
      genFix.plane3D({length, 1, length+1}, {1, length, length+1}, true); // back (z = length+1, first x, then y)
      genFix.plane3D({1, 0, length}, {length, 0, 1}, true); // top (y=0, first x, then z)

      // genFix.sideCube3D (0, 0, 0, length, sides);  

      genFix.closeHeader();
    } else if (fix == f_3DCube) {
      uint16_t width = mdl->getValue("width");
      uint16_t height =  mdl->getValue("height");
      uint16_t depth = mdl->getValue("depth");

      genFix.openHeader("3DCube%d%d%d", width, height, depth);

      genFix.cube3D(0, 0, 0, width, height, depth);

      genFix.closeHeader();

    } else if (fix == f_3DGlobe) {

      uint16_t width = mdl->getValue("width");

      genFix.openHeader("3DGlobe%d", width);

      genFix.globe3D(0, 0, 0, width);

      genFix.closeHeader();
    } else if (fix == f_3DGeodesicDome) {
// 
      uint16_t width = mdl->getValue("width");

      genFix.openHeader("3DGeodesicDome");

      genFix.geodesicDome3D(0, 0, 0);

      genFix.closeHeader();
    }

    files->filesChange();

    //reload fixture select
    ui->processUiFun("fixture");
  }

  // File openFile(const char * name) {
  //   char fileName[30] = "/";
  //   strncat(fileName, name, sizeof(fileName)-1);
  //   strncat(fileName, ".json", sizeof(fileName)-1);

  //   File f = files->open(fileName, "w");
  //   if (!f)
  //     USER_PRINTF("fixtureGen Could not open file %s for writing\n", fileName);

  //   return f;
  // }

};

static AppModFixtureGen *lfg;