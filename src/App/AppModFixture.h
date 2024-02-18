/*
   @title     StarMod
   @file      AppModFixture.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

class AppModFixture:public SysModule {

public:

  AppModFixture() :SysModule("Fixture") {};

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name);
    if (parentVar["o"] > -1000) parentVar["o"] = -1000; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

    JsonObject currentVar = ui->initCheckBox(parentVar, "on", true, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "On/Off");
        return true;
      default: return false;
    }});
    currentVar["stage"] = true;

    //logarithmic slider (10)
    currentVar = ui->initSlider(parentVar, "bri", 10, 0, 255, false , [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Brightness");
        return true;
      case f_ChangeFun: {
        uint8_t bri = var["value"];

        uint8_t result = linearToLogarithm(var, bri);

        FastLED.setBrightness(result);

        USER_PRINTF("Set Brightness to %d -> b:%d r:%d\n", var["value"].as<int>(), bri, result);
        return true;
      }
      default: return false; 
    }});
    currentVar["log"] = true; //logarithmic: not needed when using FastLED setCorrection
    currentVar["stage"] = true; //these values override model.json???

    ui->initCanvas(parentVar, "pview", UINT16_MAX, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Preview");
        ui->setComment(var, "Shows the fixture");
        // ui->setComment(var, "Click to enlarge");
        return true;
      case f_LoopFun: {
        var["interval"] =  max(lds->fixture.nrOfLeds * web->ws->count()/200, 16U)*10; //interval in ms * 10, not too fast //from cs to ms

        web->sendDataWs([this](AsyncWebSocketMessageBuffer * wsBuf) {
          uint8_t* buffer;

          buffer = wsBuf->get();

          // send leds preview to clients
          for (size_t i = 0; i < lds->fixture.nrOfLeds; i++)
          {
            buffer[i*3+5] = lds->fixture.ledsP[i].red;
            buffer[i*3+5+1] = lds->fixture.ledsP[i].green;
            buffer[i*3+5+2] = lds->fixture.ledsP[i].blue;
          }
          //new values
          buffer[0] = 1; //userFun id
          buffer[1] = lds->fixture.head.x;
          buffer[2] = lds->fixture.head.y;
          buffer[3] = lds->fixture.head.y;

        }, lds->fixture.nrOfLeds * 3 + 5, true);
        return true;
      }
      default: return false;
    }});

    ui->initSelect(parentVar, "fixture", lds->fixture.fixtureNr, false ,[](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
      {
        ui->setComment(var, "Fixture to display effect on");
        JsonArray options = ui->setOptions(var);
        files->dirToJson(options, true, "D"); //only files containing D (1D,2D,3D), alphabetically, only looking for D not very destinctive though

        // ui needs to load the file also initially
        char fileName[32] = "";
        if (files->seqNrToName(fileName, var["value"])) {
          web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
        }
        return true;
      }
      case f_ChangeFun:
      {
        lds->fixture.fixtureNr = var["value"];
        lds->fixture.doMap = true;

        //remap all leds
        for (std::vector<Leds>::iterator leds=lds->fixture.ledsList.begin(); leds!=lds->fixture.ledsList.end(); ++leds) {
          leds->doMap = true;
        }

        char fileName[32] = "";
        if (files->seqNrToName(fileName, lds->fixture.fixtureNr)) {
          //send to pview a message to get file filename
          web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
        }
        return true;
      }
      default: return false; 
    }}); //fixture

    ui->initCoord3D(parentVar, "fixSize", lds->fixture.size, 0, UINT16_MAX, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        mdl->setValue(var, lds->fixture.size);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Size");
        return true;
      default: return false;
    }});

    ui->initNumber(parentVar, "fixCount", lds->fixture.nrOfLeds, 0, UINT16_MAX, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        mdl->setValue(var, lds->fixture.nrOfLeds);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Count");
        web->addResponseV(var["id"], "comment", "Max %d", NUM_LEDS_Max);
        return true;
      default: return false;
    }});

    ui->initNumber(parentVar, "fps", lds->fps, 1, 999, false , [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setComment(var, "Frames per second");
        return true;
      case f_ChangeFun:
        lds->fps = var["value"];
        return true;
      default: return false; 
    }});

    ui->initText(parentVar, "realFps", nullptr, 10, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
        web->addResponseV(var["id"], "comment", "f(%d leds)", lds->fixture.nrOfLeds);
        return true;
      default: return false;
    }});

    #ifdef STARMOD_USERMOD_WLEDAUDIO
      ui->initCheckBox(parentVar, "mHead", false, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case f_UIFun:
          ui->setLabel(var, "Moving heads");
          ui->setComment(var, "Move on GEQ");
          return true;
        case f_ChangeFun:
          if (!var["value"])
            lds->fixture.head = {0,0,0};
          return true;
        default: return false; 
      }});
    #endif

  }
};

static AppModFixture *fix;