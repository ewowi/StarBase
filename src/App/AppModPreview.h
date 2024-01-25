/*
   @title     StarMod
   @file      AppModPreview.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"


class AppModPreview:public SysModule {

public:

  AppModPreview() :SysModule("Preview") {};

  void setup() {
    SysModule::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    parentVar = ui->initAppMod(parentVar, name);

    ui->initCanvas(parentVar, "pview", UINT16_MAX, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Preview");
      web->addResponse(var["id"], "comment", "Shows the fixture");
      // web->addResponse(var["id"], "comment", "Click to enlarge");
    }, nullptr, [](JsonObject var, uint8_t rowNr) { //loopFun

      var["interval"] =  max(ledsV.nrOfLedsP * web->ws->count()/200, 16U)*10; //interval in ms * 10, not too fast //from cs to ms

      web->sendDataWs([](AsyncWebSocketMessageBuffer * wsBuf) {
        uint8_t* buffer;

        buffer = wsBuf->get();

        // send leds preview to clients
        for (size_t i = 0; i < ledsV.nrOfLedsP; i++)
        {
          buffer[i*3+5] = ledsV.ledsPhysical[i].red;
          buffer[i*3+5+1] = ledsV.ledsPhysical[i].green;
          buffer[i*3+5+2] = ledsV.ledsPhysical[i].blue;
        }
        //new values
        buffer[0] = 1; //userFun id
        // buffer[1] = ledsV.nrOfLedsP/256;
        // buffer[2] = ledsV.nrOfLedsP%256;
        // buffer[4] = max(ledsV.nrOfLedsP * SysModWeb::ws->count()/200, 16U); //interval in ms * 10, not too fast

      }, ledsV.nrOfLedsP * 3 + 5, true); 
    });
  }
};

static AppModPreview *pvw;