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

#include "SysModule.h"

class AppModFixture:public SysModule {

public:

  AppModFixture() :SysModule("Fixture") {};

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name);
    if (parentVar["o"] > -1000) parentVar["o"] = -1000; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

    ui->initCanvas(parentVar, "pview", UINT16_MAX, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Preview");
      web->addResponse(var["id"], "comment", "Shows the fixture");
      // web->addResponse(var["id"], "comment", "Click to enlarge");
    }, nullptr, [this](JsonObject var, uint8_t rowNr) { //loopFun

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
        buffer[1] = lds->leds.head.x;
        buffer[2] = lds->leds.head.y;
        buffer[3] = lds->leds.head.y;
        // buffer[1] = fixture.nrOfLeds/256;
        // buffer[2] = fixture.nrOfLeds%256;
        // buffer[4] = max(fixture.nrOfLeds * SysModWeb::ws->count()/200, 16U); //interval in ms * 10, not too fast

      }, lds->fixture.nrOfLeds * 3 + 5, true); 
    });
    ui->initSelect(parentVar, "fixture", lds->fixture.fixtureNr, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Fixture to display effect on");
      JsonArray select = web->addResponseA(var["id"], "options");
      files->dirToJson(select, true, "D"); //only files containing D (1D,2D,3D), alphabetically, only looking for D not very destinctive though

      // ui needs to load the file also initially
      char fileName[32] = "";
      if (files->seqNrToName(fileName, var["value"])) {
        web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
      }
    }, [this](JsonObject var, uint8_t) { //chFun

      lds->fixture.fixtureNr = var["value"];
      lds->doMap = true;

      char fileName[32] = "";
      if (files->seqNrToName(fileName, lds->fixture.fixtureNr)) {
        //send to pview a message to get file filename
        JsonDocument *responseDoc = web->getResponseDoc();
        responseDoc->clear(); //needed for deserializeJson?
        JsonVariant responseVariant = responseDoc->as<JsonVariant>();

        web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
        web->sendDataWs(responseVariant);
        print->printJson("fixture chFun send ws done", responseVariant); //during server startup this is not send to a client, so client refresh should also trigger this
      }
    }); //fixture

    ui->initCoord3D(parentVar, "fixSize", lds->fixture.size, 0, UINT16_MAX, true, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Size");
      // web->addResponse(var["id"], "value", fixture.size);
    });

    ui->initNumber(parentVar, "fixCount", lds->fixture.nrOfLeds, 0, UINT16_MAX, true, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Count");
      web->addResponseV(var["id"], "comment", "Max %d", NUM_LEDS_Max);
      // web->addResponse(var["id"], "value", fixture.nrOfLeds);
    });

    ui->initNumber(parentVar, "fps", lds->fps, 1, 999, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Frames per second");
    }, [this](JsonObject var, uint8_t) { //chFun
      lds->fps = var["value"];
    });

    ui->initText(parentVar, "realFps", nullptr, 10, true, [this](JsonObject var) { //uiFun
      web->addResponseV(var["id"], "comment", "f(%d leds)", lds->fixture.nrOfLeds);
    });

  }
};

static AppModFixture *fix;