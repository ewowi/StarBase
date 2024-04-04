/*
   @title     StarMod
   @file      LedModFixtureGen.h
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

//GenFix: class to provide fixture write functions and save to json file
// {
//   "name": "F_Hexagon",
//   "nrOfLeds": 216,
//   "width": 6554,
//   "height": 6554,
//   "depth": 1,
//   "outputs": [{"pin": 2,"leds": [[720,360,0],[1073,1066,0]]}], {"pin": 3,"leds": [[720,360,0],[1073,1066,0]]}]
// }
class GenFix {

public:
  char name[32] = "";

  // unsigned16 distance = 1; //cm, not used yet (to display multiple fixture, also from other devices)

  char pinSep[2]="";
  char pixelSep[2]="";

  Coord3D fixSize = {0,0,0};
  unsigned16 nrOfLeds=0;
  
  File f;

  GenFix() {
    USER_PRINTF("GenFix constructor\n");
  }

  ~GenFix() {
    USER_PRINTF("GenFix destructor\n");
  }

  void openHeader(const char * format, ...) {
    va_list args;
    va_start(args, format);

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

    USER_PRINTF("closeHeader %d-%d-%d %d\n", fixSize.x, fixSize.y, fixSize.z, nrOfLeds);
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
    g.printf(",\"width\":%d", (fixSize.x+9)/10+1); //effects run on 1 led is 1 cm mode.
    g.printf(",\"height\":%d", (fixSize.y+9)/10+1); //e.g. (110+9)/10 +1 = 11+1 = 12, (111+9)/10 +1 = 12+1 = 13
    g.printf(",\"depth\":%d", (fixSize.z+9)/10+1);

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

  void openPin(unsigned8 pin) {
    f.printf("%s{\"pin\":%d,\"leds\":[", pinSep, pin);
    strcpy(pinSep, ",");
    strcpy(pixelSep, "");
  }
  void closePin() {
    f.printf("]}");
  }

  void write3D(Coord3D pixel) {
    write3D(pixel.x, pixel.y, pixel.z);
  }

  void write3D(unsigned16 x, unsigned16 y, unsigned16 z) {
    // if (x>UINT16_MAX/2 || y>UINT16_MAX/2 || z>UINT16_MAX/2) 
    if (x>1000 || y>1000 || z>1000) 
      USER_PRINTF("write3D coord too high %d,%d,%d\n", x, y, z);
    else
    {
      f.printf("%s[%d,%d,%d]", pixelSep, x, y, z);
      strcpy(pixelSep, ",");
      fixSize.x = max((unsigned16)fixSize.x, x);
      fixSize.y = max((unsigned16)fixSize.y, y);
      fixSize.z = max((unsigned16)fixSize.z, z);
      nrOfLeds++;
    }

  }

  //utility
  unsigned u_abs(int x) {
    if (x <0) return -x;
    else return x;
  }

  void matrix(Coord3D first, Coord3D rowEnd, Coord3D colEnd, unsigned8 ip, unsigned8 pin) {

    openPin(pin);

    //advance from pixel to rowEnd
    //if rowEnd: is it serpentine or not?
    //  no: set advancing dimension back to first
    //  yes: set non advancing dimension 1 step closer to colEnd
    //determine new rowEnd
    //if rowEnd > colEnd
    //  yes: done
    //  no: go back to first step

    // Coord3D pixel = first;

    stackUnsigned8 rowDimension; //in what dimension the row will advance (x=0, y=1, z=2), now only one should differ
    if (first.x != rowEnd.x) rowDimension = 0;
    if (first.y != rowEnd.y) rowDimension = 1; //
    if (first.z != rowEnd.z) rowDimension = 2;

    stackUnsigned8 colDimension; //in what dimension the col will advance, not the rowDimension
    if (first.x != colEnd.x && rowDimension != 0) colDimension = 0; //
    if (first.y != colEnd.y && rowDimension != 1) colDimension = 1;
    if (first.z != colEnd.z && rowDimension != 2) colDimension = 2;

    bool rowValueSame = (rowDimension == 0)? first.x == colEnd.x : (rowDimension == 1)? first.y == colEnd.y : first.z == colEnd.z; 
    bool serpentine = rowValueSame;
    
    unsigned16 nrOfColumns = (colDimension == 0)? u_abs(colEnd.x - first.x) + 1 : (colDimension == 1)? u_abs(colEnd.y - first.y) + 1 : u_abs(colEnd.z - first.z) + 1;
    if (nrOfColumns % 2 == 1 && rowValueSame) //if odd nrOfCols and rowValueSame then adjust the endpoint col value to the rowEnd col value
    {
      if (rowDimension == 0)
        colEnd.x = rowEnd.x;
      else if (rowDimension == 1)
        colEnd.y = rowEnd.y;
      else if (rowDimension == 2)
        colEnd.z = rowEnd.z;
    }

    Coord3D colPixel = Coord3D{(rowDimension==0)?colEnd.x:first.x, (rowDimension==1)?colEnd.y:first.y, (rowDimension==2)?colEnd.z:first.z};
    stackUnsigned8 colNr = 0;
    while (true) {
      // colPixel is not advancing over the dimension of the row but advances over it's own dimension towards the colEnd

      Coord3D cRowStart = Coord3D{(colDimension==0)?colPixel.x:first.x, (colDimension==1)?colPixel.y:first.y, (colDimension==2)?colPixel.z:first.z};
      Coord3D cRowEnd = Coord3D{(colDimension==0)?colPixel.x:rowEnd.x, (colDimension==1)?colPixel.y:rowEnd.y, (colDimension==2)?colPixel.z:rowEnd.z};

      if (serpentine && colNr%2 != 0) {
        Coord3D temp = cRowStart;
        cRowStart = cRowEnd;
        cRowEnd = temp;
      }

      Coord3D rowPixel = cRowStart;
      while (true) {
        USER_PRINTF(" %d,%d,%d", rowPixel.x, rowPixel.y, rowPixel.z);
        write3D(rowPixel.x*10, rowPixel.y*10, rowPixel.z*10);
        
        if (rowPixel == cRowEnd) break; //end condition row
        rowPixel.advance(cRowEnd);
      }

      USER_PRINTF("\n");

      if (colPixel == colEnd) break; //end condition columns
      colPixel.advance(colEnd);
      colNr++;
    }

    closePin();
  }

  void ring(Coord3D middle, unsigned16 ledCount, unsigned8 ip, unsigned8 pin, uint16_t pan = 0, uint16_t tilt = 0, uint16_t roll = 0, bool preDiam = false) {

    openPin(pin);

    uint8_t ringDiam = 10 * ledCount / 2 / M_PI; //in mm;
    if (preDiam) {
      //check for rings241 rings which have a specific diameter
      switch (ledCount) {
        case 1: ringDiam = 0; break;
        case 8: ringDiam = 13; break;
        case 12: ringDiam = 23; break;
        case 16: ringDiam = 33; break;
        case 24: ringDiam = 43; break;
        case 32: ringDiam = 53; break;
        case 40: ringDiam = 63; break;
        case 48: ringDiam = 73; break;
        case 60: ringDiam = 83; break;
        default: ringDiam = 10 * ledCount / 2 / M_PI; //in mm
      }
    }

    Trigo trigo;

    for (int i=0; i<ledCount; i++) {
      Coord3D pixel;
      trigo.period = ledCount; //rotate uses period 360 so set to ledCount
      pixel.x = middle.x + trigo.sin(ringDiam, i);
      pixel.y = middle.y + trigo.cos(ringDiam, i);
      pixel.z = middle.z;

      write3D(trigo.rotate(pixel, middle, pan, tilt, roll, 360));
    }
    closePin();
  }

  void rings241(Coord3D middle, unsigned8 nrOfRings, bool in2out, unsigned8 ip, unsigned8 pin, uint16_t pan = 0, uint16_t tilt = 0, uint16_t roll = 0) {
    stackUnsigned8 ringsNrOfLeds[9] = {1, 8, 12, 16, 24, 32, 40, 48, 60};
    // stackUnsigned8 ringDiams[9] = {0, 13, 23, 33, 43, 53, 63, 73, 83}; //in mm
    //  {0, 0},     //0 Center Point -> 1
    //   {1, 8},     //1 -> 8
    //   {9, 20},   //2 -> 12
    //   {21, 36},   //3 -> 16
    //   {37, 60},   //4 -> 24
    //   {61, 92},   //5 -> 32
    //   {93, 132},  //6 -> 40
    //   {133, 180}, //7 -> 48
    //   {181, 240}, //8 Outer Ring -> 60   -> 241

    // stackUnsigned16 size = ringDiams[nrOfRings-1]; //size if the biggest ring
    for (int j=0; j<nrOfRings; j++) {
      stackUnsigned8 ringNrOfLeds = in2out?ringsNrOfLeds[j]:ringsNrOfLeds[nrOfRings - 1 - j];
      ring(middle, ringNrOfLeds, ip, pin, pan, tilt, roll, true); //use predefined diameters
    }
  }

  void spiral(Coord3D first, unsigned16 ledCount, unsigned8 ip, unsigned8 pin) {

    float width = 10;
    float depth = 10;
    
    openPin(pin);

    for (int i=0; i<ledCount; i++) {
      float radians = i*360/48 * DEG_TO_RAD; //48 leds per round
      stackUnsigned16 x = 10 * width/2 * (1 + sinf(radians));
      stackUnsigned16 y = 10 * i/12; //
      stackUnsigned16 z = 10 * depth/2 * (1 + cosf(radians));
      write3D(x + first.x*10, y + first.y*10, z + first.z*10);
    }

    closePin();
  }

  void wheel(Coord3D first, unsigned16 nrOfSpokes, unsigned16 ledsPerSpoke, unsigned8 ip, unsigned8 pin) {

    float size = 50 + 10 * ledsPerSpoke;
    openPin(pin);

    for (int i=0; i<nrOfSpokes; i++) {
      float radians = i*360/nrOfSpokes * DEG_TO_RAD;
      for (int j=0;j<ledsPerSpoke;j++) {
        float ringDiam = 50 + 10 * j; //in mm
        stackUnsigned16 x = size + ringDiam * sinf(radians);
        stackUnsigned16 y = size + ringDiam * cosf(radians);
        write3D(x+first.x*10,y+first.y*10, first.z*10);
      }
    }
    closePin();
  }

  //https://stackoverflow.com/questions/71816702/coordinates-of-dot-on-an-hexagon-path
  void hexagon(Coord3D first, unsigned16 ledsPerSide, unsigned8 ip, unsigned8 pin) {

    openPin(pin);

    float radius = ledsPerSide; //or float if it needs to be tuned

    Coord3D middle = (first + Coord3D{(unsigned16)radius, (unsigned16)radius, 0})*10; //in mm

    const float y = sqrtf(3)/2; // = sin(60°)
    float hexaX[7] = {1.0, 0.5, -0.5, -1.0, -0.5, 0.5, 1.0};
    float hexaY[7] = {0.0, y, y, 0, -y, -y, 0.0};

    for (forUnsigned16 i = 0; i < ledsPerSide * 6; i++) {
      float offset = 6.0f * (float)i / (float)(ledsPerSide*6);
      stackUnsigned8 edgenum = floor(offset);  // On which edge is this dot?
      offset = offset - (float)edgenum; // Retain fractional part only: offset on that edge

      // Use interpolation to get coordinates of that point on that edge
      float x = (float)middle.x + radius*10.0f * (hexaX[edgenum] + offset * (hexaX[edgenum + 1] - hexaX[edgenum]));
      float y = (float)middle.y + radius*10.0f * (hexaY[edgenum] + offset * (hexaY[edgenum + 1] - hexaY[edgenum]));
      // USER_PRINTF(" %d %f: %f,%f", edgenum, offset, x, y);

      write3D(x, y, first.z*10);

    }

    closePin();
  }

  void cone(Coord3D first, unsigned8 nrOfRings, unsigned8 ip, unsigned8 pin) {

    float width = nrOfRings * 1.5f / M_PI + 1;

    Coord3D middle;
    middle.x = first.x*10 + width*10;
    middle.z = first.z*10 + width*10;

    for (int j=0; j<nrOfRings; j++) {
      stackUnsigned8 ringNrOfLeds = (j+1) * 3;
      middle.y = first.y*10 + j*10;

      USER_PRINTF("Cone %d %d %d %d,%d,%d\n", j, nrOfRings, ringNrOfLeds, middle.x, middle.y, middle.z);
      ring(middle, ringNrOfLeds, ip, pin, 0, 90, 0); //tilt 90
    }

  }

  void cloud(Coord3D first, unsigned8 ip, unsigned8 pin) {
    //Small RL Alt Test

    stackUnsigned8 y;

    //tbd: different pins for each section!!!

    //first pin (red)
    openPin(pin);
    y = 150; for (int x = 530; x >= 0; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 110; for (int x = 90; x <= 510; x+=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 70; for (int x = 400; x >= 110; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    closePin();
    //second pin (green)
    openPin(pin);
    y = 140; for (int x = 530; x >= 0; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 100; for (int x = 90; x <= 510; x+=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 60; for (int x = 390; x >= 120; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    closePin();
    //third pin (blue)
    openPin(pin);
    y = 130; for (int x = 520; x >= 10; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 90; for (int x = 100; x <= 500; x+=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 50; for (int x = 390; x >= 140; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    closePin();
    //fourth pin (yellow)
    openPin(pin);
    y = 120; for (int x = 520; x >= 30; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 80; for (int x = 100; x <= 480; x+=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 40; for (int x = 380; x >= 240; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 30; for (int x = 250; x <= 370; x+=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 20; for (int x = 360; x >= 260; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 10; for (int x = 270; x <= 350; x+=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    y = 00; for (int x = 330; x >= 290; x-=10) write3D(x+first.x*10, y+first.y*10, first.z*10);
    closePin();
  }

  void globe(Coord3D first, unsigned8 width, unsigned8 ip, unsigned8 pin) {

    Coord3D middle;
    middle.x = first.x*10 + width*10 / 2;
    middle.z = first.z*10 + width*10 / 2;

    // loop over latitudes -> sin 0 to pi
    for (uint8_t lat = 0; lat <= width; lat++) {
      float factor = 128.0f / width; //16 => 8
      float ringDiam = (sin8(lat * factor)-128) / factor / 2.0f; // 0 .. 128 => 0 .. 128 .. 0 => 0 .. 16 .. 0 in cm

      uint8_t ledCount = ringDiam * 2 * M_PI; 

      middle.y = first.y*10 + lat*10;

      // Coord3D start = first;
      // start.y = first.y + lat;
      USER_PRINTF("Globe %d %f %d %d,%d,%d\n", lat, ringDiam, ledCount, middle.x, middle.y, middle.z);
      ring(middle, ledCount, ip, pin, 0, 90, 0); //tilt 90
    }
  }

  // https://stackoverflow.com/questions/17705621/algorithm-for-a-geodesic-sphere
  //https://opengl.org.ru/docs/pg/0208.html
  void geodesicDome(Coord3D first, unsigned8 width, unsigned8 ip, unsigned8 pin) {
 
    stackUnsigned8 tindices[20][3] = {    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},       {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},       {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };

    openPin(pin);

    for (int i=0; i<20; i++) {
      write3D(tindices[i][0]*10 + first.x*10, tindices[i][1]*10 + first.y*10, tindices[i][2]*10 + first.z*10);
    }

    closePin();
  }

};

enum Fixtures
{
  f_Matrix,
  f_Ring,
  f_Rings241,
  f_Spiral,
  f_Wheel,
  f_Hexagon,
  f_Cone,
  f_Cloud,
  f_Wall,
  f_6Rings,
  f_SpaceStation,
  f_Human,
  f_Globe,
  f_GeodesicDome,
  fixtureCount
};

class LedModFixtureGen:public SysModule {

public:

  LedModFixtureGen() :SysModule("Fixture Generator") {};

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 6302); //created as a usermod, not an appmod to have it in the usermods tab
    parentVar["s"] = true; //setup

    ui->initSelect(parentVar, "fixtureGen", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Fixture");
        ui->setComment(var, "Type of fixture");
        JsonArray options = ui->setOptions(var); //See enum Fixtures for order of options
        options.add("Matrix");
        options.add("Ring");
        options.add("Rings241");
        options.add("Spiral");
        options.add("Wheel");
        options.add("Hexagon");
        options.add("Cone");
        options.add("Cloud");
        options.add("Wall");
        options.add("6Rings");
        options.add("SpaceStation");
        options.add("Human");
        options.add("Globe");
        options.add("GeodesicDome WIP");
        return true;
      }
      case f_ChangeFun:
        this->fixtureGenChFun();
        return true;
      default: return false; 
    }}); //fixtureGen

    ui->initCheckBox(parentVar, "panels", false, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setComment(var, "Show panels");
        return true;
      case f_ChangeFun: {
        this->fixtureGenChFun();
        return true; }
      default: return false;
    }});

	// gpio2 seems to be a safe choice on all esp32 variants
//     ui->initText(parentVar, "pinList", "2", 32, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
//       case f_UIFun:
//         ui->setComment(var, "One or more e.g. 12,13,14");
//         return true;
// #if 0		// @ewowi did not get this to work
//       case f_ValueFun:
//         #if CONFIG_IDF_TARGET_ESP32 && (defined(BOARD_HAS_PSRAM) || defined(ARDUINO_ESP32_PICO)) // 
//           ui->setValue(var, "2");  // gpio16 is reserved on pico and on esp32 with PSRAM
//         #elif CONFIG_IDF_TARGET_ESP32S3
//           ui->setValue(var, "21");  // gpio21 = builtin neopixel on some -S3 boards
//         #elif CONFIG_IDF_TARGET_ESP32C3
//           ui->setValue(var, "10");  // gpio10 = builtin neopixel on some -C3 boards
//         #else
//           ui->setValue(var, "16");  // default on universal shield (classic esp32, or esp32-S2)
//         #endif
//         return true;
// #endif
//       default: return false;
//     }});

    ui->initButton(parentVar, "generate", false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ChangeFun: {

        char fileName[32]; 
        generateChFun(var, fileName);
        ui->callVarFun("fixture", UINT8_MAX, f_UIFun); //reload fixture select

        uint8_t value = ui->selectOptionToValue("fixture", fileName);
        if (value != UINT8_MAX)
          mdl->setValue("fixture", value);

        return true; }
      default: return false;
    }});

  } //setup

  void loop() {
    // SysModule::loop();
  }

  //generate dynamic html for fixture controls
  void fixtureGenChFun() {

    JsonObject fixtureGenVar = mdl->findVar("fixtureGen");
    JsonObject panelVar = mdl->findVar("panels");

    // JsonObject parentVar = mdl->findVar(var["id"]); //local parentVar
    stackUnsigned8 fgValue = fixtureGenVar["value"];

    fixtureGenVar.remove("n"); //tbd: we should also remove the varFun !!

    // mdl->varPreDetails(fixtureGenVar);

    JsonObject parentVar = fixtureGenVar;
    if (panelVar["value"].as<bool>()) {

      parentVar = ui->initTable(fixtureGenVar, "pnlTbl", nullptr, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case f_UIFun:
          ui->setLabel(var, "Panels");
          ui->setComment(var, "List of fixtures");
          return true;
        case f_AddRow:
          web->getResponseObject()["addRow"]["rowNr"] = rowNr;
          return true;
        case f_DelRow:
          // web->getResponseObject()["addRow"]["rowNr"] = rowNr;
          return true;
        default: return false;
      }});

      ui->initCoord3D(parentVar, "pnlFirst", {0,0,0}, 0, NUM_LEDS_Max, false, [fgValue](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case f_UIFun:
          //show Top Left for all fixture except Matrix as it has its own
          if (fgValue == f_Matrix)
            ui->setLabel(var, "First LED");
          else 
            ui->setLabel(var, "Top left");
          return true;
        default: return false;
      }});

    } //if panels


    if (fgValue == f_Matrix) {

      ui->initCoord3D(parentVar, "mrxRowEnd", {7,0,0}, 0, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case f_UIFun:
          ui->setLabel(var, "Row end");
          ui->setComment(var, "-> Orientation");
          return true;
        default: return false;
      }});

      ui->initCoord3D(parentVar, "mrxColEnd", {7,7,0}, 0, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case f_UIFun:
          ui->setLabel(var, "Column end");
          ui->setComment(var, "Last LED -> nrOfLeds, Serpentine");
          return true;
        default: return false;
      }});

      if (panelVar["value"].as<bool>()) {

        ui->initSelect(fixtureGenVar, "fixPreset", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case f_UIFun: {
            ui->setLabel(var, "Preset");
            JsonArray options = ui->setOptions(var);
            options.add("None");
            options.add("16x16");
            options.add("4x16x16");
            options.add("4x32x8");
            options.add("Human Sized Cube");
            options.add("CubeBox");
            options.add("Cube3D");
            options.add("Sticks");
            return true; }
          case f_ChangeFun: {
            stackUnsigned8 optionNr = 1; // 0 is none, maintain the same order here as the options
            stackUnsigned8 panel = 0;
            if (var["value"] == optionNr++) { //16x16
              stackUnsigned8 lengthMinOne = 15;
              panel = 0; mdl->setValue("pnlFirst", Coord3D{0,0,0}, panel++); 
              panel = 0; mdl->setValue("mrxRowEnd", Coord3D{0,lengthMinOne,0}, panel++);
              panel = 0; mdl->setValue("mrxColEnd", Coord3D{lengthMinOne,lengthMinOne,0}, panel++);
              panel = 0; mdl->setValue("fixPin", 2, panel++); // default per board...
            }
            if (var["value"] == optionNr++) { //4x16x16
              stackUnsigned8 lengthMinOne = 15; stackUnsigned8 size = lengthMinOne + 1;
              panel = 0; mdl->setValue("pnlFirst", Coord3D{0,0,0}, panel++); mdl->setValue("pnlFirst", Coord3D{16,0,0}, panel++); mdl->setValue("pnlFirst", Coord3D{0,16,0}, panel++); mdl->setValue("pnlFirst", Coord3D{16,16,0}, panel++); 
              panel = 0; mdl->setValue("mrxRowEnd", Coord3D{0,lengthMinOne,0}, panel++); mdl->setValue("mrxRowEnd", Coord3D{16,lengthMinOne,0}, panel++); mdl->setValue("mrxRowEnd", Coord3D{0,31,0}, panel++); mdl->setValue("mrxRowEnd", Coord3D{16,31,0}, panel++); 
              panel = 0; mdl->setValue("mrxColEnd", Coord3D{lengthMinOne,lengthMinOne,0}, panel++); mdl->setValue("mrxColEnd", Coord3D{31,lengthMinOne,0}, panel++); mdl->setValue("mrxColEnd", Coord3D{lengthMinOne,31,0}, panel++); mdl->setValue("mrxColEnd", Coord3D{31,31,0}, panel++); 
              panel = 0; mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); 
            }
            if (var["value"] == optionNr++) { //4x32x8
              stackUnsigned8 lengthMinOne = 31;
              panel = 0; mdl->setValue("pnlFirst", Coord3D{lengthMinOne,lengthMinOne,0}, panel++); mdl->setValue("pnlFirst", Coord3D{lengthMinOne,23,0}, panel++); mdl->setValue("pnlFirst", Coord3D{lengthMinOne,15,0}, panel++); mdl->setValue("pnlFirst", Coord3D{lengthMinOne,7,0}, panel++);
              panel = 0; mdl->setValue("mrxRowEnd", Coord3D{lengthMinOne,24,0}, panel++); mdl->setValue("mrxRowEnd", Coord3D{lengthMinOne,16,0}, panel++); mdl->setValue("mrxRowEnd", Coord3D{lengthMinOne,8,0}, panel++); mdl->setValue("mrxRowEnd", Coord3D{lengthMinOne,0,0}, panel++);
              panel = 0; mdl->setValue("mrxColEnd", Coord3D{0,lengthMinOne,0}, panel++); mdl->setValue("mrxColEnd", Coord3D{0,23,0}, panel++); mdl->setValue("mrxColEnd", Coord3D{0,15,0}, panel++); mdl->setValue("mrxColEnd", Coord3D{0,7,0}, panel++);
              panel = 0; mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); 
            }
            else if (var["value"] == optionNr++) { //Human Sized Cube
              stackUnsigned8 length = 20; stackUnsigned8 size = length + 1;
              panel = 0; mdl->setValue("pnlFirst", Coord3D{1,1,size}, panel++); mdl->setValue("pnlFirst", Coord3D{0,1,1}, panel++); mdl->setValue("pnlFirst", Coord3D{1,1,0}, panel++); mdl->setValue("pnlFirst", Coord3D{size,1,1}, panel++); mdl->setValue("pnlFirst", Coord3D{1,0,1}, panel++);
              panel = 0; mdl->setValue("mrxRowEnd", Coord3D{1,length,size}, panel++); mdl->setValue("mrxRowEnd", Coord3D{0,length,1}, panel++); mdl->setValue("mrxRowEnd", Coord3D{1,length,0}, panel++); mdl->setValue("mrxRowEnd", Coord3D{size,length,1}, panel++); mdl->setValue("mrxRowEnd", Coord3D{1,0,length}, panel++);
              panel = 0; mdl->setValue("mrxColEnd", Coord3D{length,length,size}, panel++); mdl->setValue("mrxColEnd", Coord3D{0,length,length}, panel++); mdl->setValue("mrxColEnd", Coord3D{length,length,0}, panel++); mdl->setValue("mrxColEnd", Coord3D{size,length,length}, panel++); mdl->setValue("mrxColEnd", Coord3D{length,0,length}, panel++);
              panel = 0; mdl->setValue("fixPin", 16, panel++); mdl->setValue("fixPin", 14, panel++); mdl->setValue("fixPin", 32, panel++); mdl->setValue("fixPin", 3, panel++); mdl->setValue("fixPin", 15, panel++);
            }
            else if (var["value"] == optionNr++) { //Cube 6 x 8 x 8
              stackUnsigned8 length = 8; stackUnsigned8 size = length + 1;
              panel = 0; mdl->setValue("pnlFirst", Coord3D{1,1,0}, panel++); mdl->setValue("pnlFirst", Coord3D{length, size, length}, panel++); mdl->setValue("pnlFirst", Coord3D{size, 1, 1}, panel++); mdl->setValue("pnlFirst", Coord3D{0, length, length}, panel++); mdl->setValue("pnlFirst", Coord3D{length, 1, size}, panel++); mdl->setValue("pnlFirst", Coord3D{1, 0, length}, panel++);
              panel = 0; mdl->setValue("mrxRowEnd", Coord3D{1,length,0}, panel++); mdl->setValue("mrxRowEnd", Coord3D{1, size, length}, panel++); mdl->setValue("mrxRowEnd", Coord3D{size, 1, length}, panel++); mdl->setValue("mrxRowEnd", Coord3D{0, 1, length}, panel++); mdl->setValue("mrxRowEnd", Coord3D{1, 1, size}, panel++); mdl->setValue("mrxRowEnd", Coord3D{length, 0, length}, panel++);
              panel = 0; mdl->setValue("mrxColEnd", Coord3D{length,length,0}, panel++); mdl->setValue("mrxColEnd", Coord3D{1, size, 1}, panel++); mdl->setValue("mrxColEnd", Coord3D{size, length, length}, panel++); mdl->setValue("mrxColEnd", Coord3D{0, 1, 1}, panel++); mdl->setValue("mrxColEnd", Coord3D{1, length, size}, panel++); mdl->setValue("mrxColEnd", Coord3D{length, 0, 1}, panel++);
              panel = 0; mdl->setValue("fixPin", 12, panel++); mdl->setValue("fixPin", 12, panel++); mdl->setValue("fixPin", 13, panel++); mdl->setValue("fixPin", 13, panel++); mdl->setValue("fixPin", 14, panel++); mdl->setValue("fixPin", 14, panel++);
            }
            else if (var["value"] == optionNr++) { //Cube 3D
              stackUnsigned8 length = 8; stackUnsigned8 size = length -1;
              for (forUnsigned8 panel=0; panel < length; panel++) {
                mdl->setValue("pnlFirst", Coord3D{0,0,(unsigned16)panel}, panel);
                mdl->setValue("mrxRowEnd", Coord3D{0,size,(unsigned16)panel}, panel);
                mdl->setValue("mrxColEnd", Coord3D{size,size,(unsigned16)panel}, panel);
                mdl->setValue("fixPin", 12, panel);
              }
            }
            else if (var["value"] == optionNr++) { //Sticks
              stackUnsigned8 length = 16;
              stackUnsigned8 height = 54;
              for (forUnsigned8 panel=0; panel < length; panel++) {
                mdl->setValue("pnlFirst", Coord3D{(unsigned16)(panel*5), height, 0}, panel);
                mdl->setValue("mrxRowEnd", Coord3D{(unsigned16)(panel*5), height, 0}, panel);
                mdl->setValue("mrxColEnd", Coord3D{(unsigned16)(panel*5), 0, 0}, panel);
                mdl->setValue("fixPin", 12, panel);
              }
            }
            return true; }
          default: return false; 
        }});
      }
    }
    else if (fgValue == f_Ring) {
      ui->initNumber(parentVar, "fixLeds", 24, 1, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case f_UIFun:
          ui->setLabel(var, "Leds");
          return true;
        default: return false; 
      }});

      if (panelVar["value"].as<bool>()) {

        ui->initSelect(fixtureGenVar, "fixPreset", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case f_UIFun: {
            ui->setLabel(var, "Preset");
            JsonArray options = ui->setOptions(var);
            options.add("None");
            options.add("Olympic");
            options.add("Audi");
            return true; }
          case f_ChangeFun: {
            stackUnsigned8 optionNr = 1; // 0 is none, maintain the same order here as the options
            stackUnsigned8 panel = 0;
            if (var["value"] == optionNr++) { //Olympic
              panel = 0; mdl->setValue("pnlFirst", Coord3D{0,0,0}, panel++); mdl->setValue("pnlFirst", Coord3D{10,0,0}, panel++); mdl->setValue("pnlFirst", Coord3D{20,0,0}, panel++); mdl->setValue("pnlFirst", Coord3D{5,3,0}, panel++); mdl->setValue("pnlFirst", Coord3D{15,3,0}, panel++); 
              panel = 0; mdl->setValue("fixLeds", 24, panel++); mdl->setValue("fixLeds", 24, panel++); mdl->setValue("fixLeds", 24, panel++); mdl->setValue("fixLeds", 24, panel++); mdl->setValue("fixLeds", 24, panel++); 
              panel = 0; mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); // default per board...
            }
            if (var["value"] == optionNr++) { //Audi
              panel = 0; mdl->setValue("pnlFirst", Coord3D{0,0,0}, panel++); mdl->setValue("pnlFirst", Coord3D{6,0,0}, panel++); mdl->setValue("pnlFirst", Coord3D{12,0,0}, panel++); mdl->setValue("pnlFirst", Coord3D{18  ,0,0}, panel++);
              panel = 0; mdl->setValue("fixLeds", 24, panel++); mdl->setValue("fixLeds", 24, panel++); mdl->setValue("fixLeds", 24, panel++); mdl->setValue("fixLeds", 24, panel++); 
              panel = 0; mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); mdl->setValue("fixPin", 2, panel++); // default per board...
            }
            return true; }
          default: return false; 
        }});
      }
    }
    else if (fgValue == f_Rings241) {
      ui->initNumber(parentVar, "nrOfRings", 9, 1, 9);
      ui->initCheckBox(parentVar, "in2out", true);
    }
    else if (fgValue == f_Spiral) {
      ui->initNumber(parentVar, "fixLeds", 64, 1, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case f_UIFun:
          ui->setLabel(var, "Leds");
          return true;
        default: return false; 
      }});
    }
    else if (fgValue == f_Wheel) {
      ui->initNumber(parentVar, "nrOfSpokes", 36, 1, 360);
      ui->initNumber(parentVar, "ledsPerSpoke", 24, 1, 360);
    }
    else if (fgValue == f_Hexagon) {
      ui->initNumber(parentVar, "ledsPerSide", 12, 1, 255);

      if (panelVar["value"].as<bool>()) {

        ui->initSelect(fixtureGenVar, "fixPreset", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case f_UIFun: {
            ui->setLabel(var, "Preset");
            JsonArray options = ui->setOptions(var);
            options.add("None");
            options.add("HexaWall");
            return true; }
          case f_ChangeFun: {
            stackUnsigned8 optionNr = 1; // 0 is none, maintain the same order here as the options
            stackUnsigned8 panel = 0;
            if (var["value"] == optionNr++) { //HexaWall
              panel = 0; 
              mdl->setValue("pnlFirst", Coord3D{0,0,0}, panel++); 
              mdl->setValue("pnlFirst", Coord3D{10,6,0}, panel++); 
              mdl->setValue("pnlFirst", Coord3D{10,18,0}, panel++); 
              mdl->setValue("pnlFirst", Coord3D{20,0,0}, panel++); 
              mdl->setValue("pnlFirst", Coord3D{30,6,0}, panel++); 
              mdl->setValue("pnlFirst", Coord3D{40,12,0}, panel++); 
              mdl->setValue("pnlFirst", Coord3D{40,24,0}, panel++); 
              mdl->setValue("pnlFirst", Coord3D{50,6,0}, panel++); 
              mdl->setValue("pnlFirst", Coord3D{60,0,0}, panel++); 

              for (forUnsigned8 panel = 0; panel < 9; panel++) {
                mdl->setValue("ledsPerSide", 6, panel);
                mdl->setValue("fixPin", 2, panel);
              }
            }
            return true; }
          default: return false; 
        }});
      }
    }
    else if (fgValue == f_Human) {

      if (panelVar["value"].as<bool>()) {

        ui->initSelect(fixtureGenVar, "fixPreset", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case f_UIFun: {
            ui->setLabel(var, "Preset");
            JsonArray options = ui->setOptions(var);
            options.add("Body1");
            options.add("Body2");
            return true; }
          default: return false; 
        }});
      }

    }
    else if (fgValue == f_Cone) {
      ui->initNumber(parentVar, "nrOfRings", 24, 1, 360);
    }
    else if (fgValue == f_Globe) {
      ui->initNumber(parentVar, "width", 24, 1, 48);
    }



    ui->initNumber(parentVar, "fixIP", WiFi.localIP()[3], 1, 256, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "IP");
        ui->setComment(var, "Super-Sync WIP");
        return true;
      default: return false; 
    }});

    ui->initPin(parentVar, "fixPin", 2, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Pin");

        //tbd: move logic to pinMgr and create initPin
        JsonArray options = ui->setOptions(var);
        
        for (forUnsigned8 pinNr = 0; pinNr < NUM_DIGITAL_PINS; pinNr++) {
          char text[32];
          itoa(pinNr, text, 10);
          if (digitalPinIsValid(pinNr)) {

            #if defined(CONFIG_IDF_TARGET_ESP32S2)
              if ((pinNr > 18 && pinNr < 21) || (pinNr > 21 && pinNr < 33)) strcat(text, " 🟣"); else 
            #elif defined(CONFIG_IDF_TARGET_ESP32S3)
              if ((pinNr > 18 && pinNr < 21) || (pinNr > 21 && pinNr < 33)) strcat(text, " 🟣"); else 
            #elif defined(CONFIG_IDF_TARGET_ESP32C3)
              if ((pinNr > 11 && pinNr < 18) || (pinNr > 17 && pinNr < 20)) strcat(text, " 🟣"); else 
            #elif defined(ESP32)
              if (pinNr > 5 && pinNr < 12) strcat(text, " 🟣"); else 
            #else //???
            #endif

            if (!digitalPinCanOutput(pinNr)) 
              strcat(text, " 🟠"); //read only
            else
              strcat(text, " 🟢"); //io

            //results in crashes
            // if (digitalPinToRtcPin(pinNr)) strcat(text, " 🟢"); else strcat(text, " 🔴"); //error: 'RTC_GPIO_IS_VALID_GPIO' was not declared in this scope
            // if (digitalPinToDacChannel(pinNr)) strcat(text, " 🟢"); else strcat(text, " 🔴"); //error: 'DAC_CHANNEL_1_GPIO_NUM' was not declared in this scope

            //not so relevant
            // if (digitalPinToAnalogChannel(pinNr)) strcat(text, " 🟣");
            // if (digitalPinToTouchChannel(pinNr)) strcat(text, " 🟤");
          }
          else 
            strcat(text, " 🔴"); //not valid
          options.add(JsonString(text, JsonString::Copied));
        }
        return true; }
      case f_ChangeFun: {
        //set remaining rows to same pin
        JsonArray valArray = mdl->varValArray(var);

        stackUnsigned8 thisVal = var["value"];
        stackUnsigned8 rowNrL = 0;
        for (JsonVariant val: valArray) {
          if (rowNrL > rowNr)
            mdl->setValue(var, valArray[rowNr].as<unsigned8>(), rowNrL);
          rowNrL++;
        }
        return true; }
      default: return false; 
    }});

    mdl->varPostDetails(fixtureGenVar, UINT8_MAX);

  }

  //tbd: move to utility functions
  char *removeSpaces(char *str) 
  { 
      int i = 0, j = 0; 
      while (str[i]) 
      { 
          if (str[i] != ' ') 
          str[j++] = str[i]; 
          i++; 
      } 
      str[j] = '\0'; 
      return str; 
  } 

  void getPanels(const char * fileName, std::function<void(GenFix *, unsigned8)> genFun) {
    GenFix genFix;

    JsonObject presetsVar = mdl->findVar("fixPreset");
    stackUnsigned8 presets = mdl->getValue(presetsVar);

    //set header and file name
    if (presets != 0) {
      JsonArray options = ui->getOptions(presetsVar);

      char text[32] = "F_";
      strncat(text, options[presets], 31);
      removeSpaces(text);

      genFix.openHeader(text);

      ui->clearOptions(presetsVar);
    }
    else {
      // stackUnsigned16 ledCount = mdl->getValue("fixLeds");
      genFix.openHeader(fileName);
    }

    JsonVariant pnlFirstValue = mdl->findVar("pnlFirst")["value"];

    if (pnlFirstValue.is<JsonArray>()) {
      stackUnsigned8 rowNr = 0;
      for (JsonVariant firstValueRow: pnlFirstValue.as<JsonArray>()) {
        genFun(&genFix, rowNr);
        rowNr++;
      }
    } else {
      genFun(&genFix, UINT8_MAX);
    }

    genFix.closeHeader();
  }
 
  void generateChFun(JsonObject var, char * fileName) {

    stackUnsigned8 fgValue = mdl->getValue("fixtureGen");

    if (fgValue == f_Matrix) {

      Coord3D size = (mdl->getValue("mrxColEnd").as<Coord3D>() - mdl->getValue("pnlFirst").as<Coord3D>()) + Coord3D{1,1,1};
      print->fFormat(fileName, 31, "F_Matrix%d%d%d", size.x, size.y, size.z);

      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->matrix(mdl->getValue("pnlFirst", rowNr), mdl->getValue("mrxRowEnd", rowNr), mdl->getValue("mrxColEnd", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_Ring) {

      print->fFormat(fileName, 31, "F_Ring%d", mdl->getValue("fixLeds").as<unsigned8>());

      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        Coord3D middle = mdl->getValue("pnlFirst", rowNr);
        uint16_t ledCount = mdl->getValue("fixLeds", rowNr);
        uint8_t ringDiam = 10 * ledCount / 2 / M_PI + 10; //in mm
        middle.x = middle.x*10 + ringDiam;
        middle.y = middle.y*10 + ringDiam;
        
        genFix->ring(middle, ledCount, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_Rings241) {

      print->fFormat(fileName, 31, "F_Rings241-%d", mdl->getValue("nrOfRings").as<unsigned8>());

      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        Coord3D middle = mdl->getValue("pnlFirst", rowNr);
        uint16_t nrOfLeds = mdl->getValue("fixLeds", rowNr);
        uint8_t ringDiam = 10 * 60 / 2 / M_PI; //in mm
        middle.x = middle.x*10 + ringDiam;
        middle.y = middle.y*10 + ringDiam;
        genFix->rings241(middle, mdl->getValue("nrOfRings", rowNr), mdl->getValue("in2out", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_Spiral) {

      print->fFormat(fileName, 31, "F_Spiral%d", mdl->getValue("fixLeds").as<unsigned8>());

      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->spiral(mdl->getValue("pnlFirst", rowNr), mdl->getValue("fixLeds", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_Wheel) {

      print->fFormat(fileName, 31, "F_Wheel%d%d", mdl->getValue("nrOfSpokes").as<unsigned8>(), mdl->getValue("ledsPerSpoke").as<unsigned8>());
      
      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->wheel(mdl->getValue("pnlFirst", rowNr), mdl->getValue("nrOfSpokes", rowNr), mdl->getValue("ledsPerSpoke", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_Hexagon) {

      strcpy(fileName, "F_Hexagon");
      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->hexagon(mdl->getValue("pnlFirst", rowNr), mdl->getValue("ledsPerSide", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_Cone) {

      print->fFormat(fileName, 31, "F_Cone%d", mdl->getValue("nrOfRings").as<unsigned8>());

      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->cone(mdl->getValue("pnlFirst", rowNr), mdl->getValue("nrOfRings", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_Cloud) {

      strcpy(fileName, "F_Cloud5416");
      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->cloud(mdl->getValue("pnlFirst", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_Wall) {

      strcpy(fileName, "F_Wall");
      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->rings241(Coord3D{110,110,0}, 9, true, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));

        genFix->matrix(Coord3D{19,0,0}, Coord3D{19,8,0}, Coord3D{27,0,0}, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
        genFix->matrix(Coord3D{0,19,0}, Coord3D{0,25,0}, Coord3D{50,19,0}, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));

        genFix->ring(Coord3D{190+80,80+80,0}, 48, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));

        // genFix.spiral(240, 0, 0, 48);
      });

    } else if (fgValue == f_6Rings) {

      strcpy(fileName, "F_6Rings");
      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->rings241(Coord3D{110,110,0}, 9, true, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
        genFix->rings241(Coord3D{0,110,110}, 9, true, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr), 90); //pan 90
        genFix->rings241(Coord3D{110,110,220}, 9, true, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
        genFix->rings241(Coord3D{220,110,110}, 9, true, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr), 90); // pan 90
        genFix->rings241(Coord3D{110,0,110}, 9, true, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr), 0, 90); // tilt 90
        genFix->rings241(Coord3D{110,220,110}, 9, true, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr), 0, 90); // tilt 90
      });

    } else if (fgValue == f_SpaceStation) {

      strcpy(fileName, "F_SpaceStation");
      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        Trigo trigo;
        for (int i=0; i<360; i+=20) {
          uint8_t ringDiam = 50;
          uint8_t issDiam = 150;
          Coord3D middle = Coord3D{ringDiam + issDiam + trigo.sin(issDiam, i), ringDiam + issDiam + trigo.cos(issDiam, i), ringDiam};
          genFix->ring(middle, 12, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr), 0, 90, 360-i); // tilt 90 then roll a bit
        }
      });

    } else if (fgValue == f_Human) {

      strcpy(fileName, "F_Human");
      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {

        //total dimensions width: 10 + 18 + 10
        // height 18 + 5 + 
        Coord3D headSize = {18, 18, 1};
        Coord3D armSize = {12, 5, 1};
        Coord3D legSize = {5, 20, 1};

        // head
        genFix->rings241({190,110,0}, 9, true, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));

        //arms
        genFix->matrix(Coord3D{0,headSize.y + 2,0}, Coord3D{0,headSize.y + 2 + armSize.y,0}, Coord3D{armSize.x,headSize.y + 2,0}, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
        genFix->matrix(Coord3D{armSize.x + 14,headSize.y + 2,0}, Coord3D{armSize.x + 14,headSize.y + 2 + armSize.y,0}, Coord3D{38,headSize.y + 2,0}, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));

        //legs
        genFix->matrix(Coord3D{10, 28, 0}, Coord3D{10, 50, 0}, Coord3D{15,28,0}, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
        genFix->matrix(Coord3D{23, 28, 0}, Coord3D{23, 50, 0}, Coord3D{28,28,0}, mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));


      });

    } else if (fgValue == f_Globe) {

      print->fFormat(fileName, 31, "F_Globe%d", mdl->getValue("width").as<unsigned8>());

      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->globe(mdl->getValue("pnlFirst", rowNr), mdl->getValue("width", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    } else if (fgValue == f_GeodesicDome) {

      strcpy(fileName, "F_GeodesicDome");
      getPanels(fileName, [](GenFix * genFix, unsigned8 rowNr) {
        genFix->geodesicDome(mdl->getValue("pnlFirst", rowNr), mdl->getValue("width", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
      });

    }

    files->filesChanged = true;
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

extern LedModFixtureGen *lfg;