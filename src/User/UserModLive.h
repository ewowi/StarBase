/*
   @title     StarBase
   @file      UserModLive.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors, asmParser © https://github.com/hpwit/ASMParser
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

// #define __RUN_CORE 0
#include "parser.h"

long time1;
static float _min = 9999;
static float _max = 0;
static uint32_t _nb_stat = 0;
static float _totfps;

static void show1()
{
    // SKIPPED: check nargs (must be 3 because arg[0] is self)
    long time2 = ESP.getCycleCount();
    // driver.showPixels(WAIT);

    float k = (float)(time2 - time1) / 240000000;
    float fps = 1 / k;
    _nb_stat++;
    if (_min > fps && fps > 10 && _nb_stat > 10)
        _min = fps;
    if (_max < fps && fps < 200 && _nb_stat > 10)
        _max = fps;
    if (_nb_stat > 10)
        _totfps += fps;
    ppf("current fps:%.2f  average:%.2f min:%.2f max:%.2f\r\n", fps, _totfps / (_nb_stat - 10), _min, _max);
    time1 = ESP.getCycleCount();

    // SKIPPED: check that both v1 and v2 are int numbers
    // RETURN_VALUE(VALUE_FROM_INT(0), rindex);
}

static void show2() {
  // ppf("show 2\n"); //test without print - works
}

class UserModLive:public SysModule {

public:

  Parser p = Parser();

  UserModLive() :SysModule("Live") {
    isEnabled = false; //need to enable after fresh setup
  };

  void setup() override {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6310);

    ui->initSelect(parentVar, "script", UINT16_MAX, false ,[this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        // ui->setComment(var, "Fixture to display effect on");
        JsonArray options = ui->setOptions(var);
        options.add("None");
        files->dirToJson(options, true, ".sc"); //only files containing F(ixture), alphabetically

        return true; }
      case onChange: {
        //set script
        uint8_t fileNr = var["value"];

        SCExecutable._kill(); //kill any old tasks

        ppf("%s script f:%d f:%d\n", name, funType, fileNr);

        if (fileNr > 0) { //not None

          fileNr--;  //-1 as none is no file

          char fileName[32] = "";

          files->seqNrToName(fileName, fileNr, ".sc");

          // ppf("%s script f:%d f:%d\n", name, funType, fileNr);

          if (strcmp(fileName, "") != 0) {

            File f = files->open(fileName, "r");
            if (!f)
              ppf("UserModLive setup script  open %s for %s failed", fileName, "r");
            else {

              string script = string(f.readString().c_str());

              ppf("%s\n", script);

              if (p.parse_c(&script))
              {
                  SCExecutable.executeAsTask("main");
              }
              f.close();
            }
          }
          else
            ppf("UserModLive setup file for %d not found", fileNr);
        }
        return true; }
      default: return false; 
    }}); //fixture

    // ui->initButton

    addExternal("show1", externalType::function, (void *)&show1);
    addExternal("show2", externalType::function, (void *)&show2);

    // addExternal("test", externalType::function, (void *)&test); //compile error ...

  }

  void test() {
    ppf("hello test world\n");
  }

};

extern UserModLive *live;


//asm_parser.h:325:1: warning: control reaches end of non-void function 