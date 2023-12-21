/*
   @title     StarMod
   @file      AppModLeds.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModule.h"

#include "AppLedsV.h"
#include "AppEffects.h"
#ifdef USERMOD_E131
  #include "../User/UserModE131.h"
#endif

#include <vector>
#include "FastLED.h"

// #define FASTLED_RGBW

//https://www.partsnotincluded.com/fastled-rgbw-neopixels-sk6812/
inline uint16_t getRGBWsize(uint16_t nleds){
	uint16_t nbytes = nleds * 4;
	if(nbytes % 3 > 0) return nbytes / 3 + 1;
	else return nbytes / 3;
}

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

class AppModLeds:public SysModule {

public:
  bool newFrame = false; //for other modules (DDP)

  uint16_t fps = 60;
  unsigned long lastMappingMillis = 0;
  bool doMap = false;
  Effects effects;

  AppModLeds() :SysModule("Leds") {};

  void setup() {
    SysModule::setup();
    USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

    parentVar = ui->initModule(parentVar, name);
    JsonObject currentVar;

    currentVar = ui->initCheckBox(parentVar, "on", true, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "On/Off");
    });
    currentVar["stage"] = true;

    //logarithmic slider (10)
    currentVar = ui->initSlider(parentVar, "bri", 10, 0, 255, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Brightness");
    }, [](JsonObject var, uint8_t) { //chFun
      uint8_t bri = var["value"];

      uint8_t result = linearToLogarithm(var, bri);

      FastLED.setBrightness(result);

      USER_PRINTF("Set Brightness to %d -> b:%d r:%d\n", var["value"].as<int>(), bri, result);
    });
    currentVar["log"] = true; //logarithmic
    currentVar["stage"] = true; //these values override model.json???

    ui->initCanvas(parentVar, "pview", -1, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Preview");
      web->addResponse(var["id"], "comment", "Shows the fixture");
      // web->addResponse(var["id"], "comment", "Click to enlarge");
    }, nullptr, [](JsonObject var, uint8_t* buffer) { //loopFun
      // send leds preview to clients
      for (size_t i = 0; i < buffer[0] * 256 + buffer[1]; i++)
      {
        buffer[i*3+4] = ledsP[i].red;
        buffer[i*3+4+1] = ledsP[i].green;
        buffer[i*3+4+2] = ledsP[i].blue;
      }
      //new values
      buffer[0] = ledsV.nrOfLedsP/256;
      buffer[1] = ledsV.nrOfLedsP%256;
      buffer[3] = max(ledsV.nrOfLedsP * SysModWeb::ws->count()/200, 16U); //interval in ms * 10, not too fast
    });

    JsonObject tableVar = ui->initTable(parentVar, "fxTbl", nullptr, false, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Effects");
      web->addResponse(var["id"], "comment", "List of effects (WIP)");
      JsonArray rows = web->addResponseA(var["id"], "data");
      // for (SysModule *module:modules) {

      //add value for each child
      JsonArray row = rows.createNestedArray();
      for (JsonObject childVar : var["n"].as<JsonArray>()) {
        print->printJson("fxTbl childs", childVar);
        row.add(childVar["value"]);
        //recursive
        // for (JsonObject childVar : childVar["n"].as<JsonArray>()) {
        //   print->printJson("fxTbl child childs", childVar);
        //   row.add(childVar["value"]);
        // }
      }

    });

    currentVar = ui->initSelect(parentVar, "fx", 0, false, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Effect");
      web->addResponse(var["id"], "comment", "Effect to show");
      JsonArray select = web->addResponseA(var["id"], "data");
      for (Effect *effect:effects.effects) {
        select.add(effect->name());
      }
    }, [this](JsonObject var, uint8_t rowNr) { //chFun
      doMap = effects.setEffect(var, rowNr);
    });
    currentVar["stage"] = true;


    currentVar = ui->initSelect(tableVar, "pro", 2, false, [](JsonObject var) { //uiFun.
      web->addResponse(var["id"], "label", "Projection");
      web->addResponse(var["id"], "comment", "How to project fx to fixture");
      JsonArray select = web->addResponseA(var["id"], "data");
      select.add("None"); // 0
      select.add("Random"); // 1
      select.add("Distance from point"); //2
      select.add("Distance from center"); //3
      select.add("Mirror"); //4
      select.add("Reverse"); //5
      select.add("Multiply"); //6
      select.add("Kaleidoscope"); //7
      select.add("Fun"); //8
    }, [this](JsonObject var, uint8_t rowNr) { //chFun

      ledsV.projectionNr = var["value"][rowNr];
      doMap = true;

    });
    currentVar["stage"] = true;

    ui->initNumber(tableVar, "pointX", 0, 0, 127, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Depends on how much leds fastled has configured");
    });
    ui->initNumber(tableVar, "pointY", 0, 0, 127);
    ui->initNumber(tableVar, "pointZ", 0, 0, 127);
    // ui->initNumber(tableVar, "endX", 0, 0, 127);
    // ui->initNumber(tableVar, "endY", 0, 0, 127);
    // ui->initNumber(tableVar, "endZ", 0, 0, 127);

    ui->initSelect(parentVar, "fixture", 0, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Fixture to display effect on");
      JsonArray select = web->addResponseA(var["id"], "data");
      files->dirToJson(select, true, "D"); //only files containing D (1D,2D,3D), alphabetically, only looking for D not very destinctive though

      // ui needs to load the file also initially
      char fileName[32] = "";
      if (files->seqNrToName(fileName, var["value"])) {
        web->addResponse("pview", "file", fileName);
      }
    }, [this](JsonObject var, uint8_t) { //chFun

      ledsV.fixtureNr = var["value"];
      doMap = true;

      char fileName[32] = "";
      if (files->seqNrToName(fileName, ledsV.fixtureNr)) {
        //send to pview a message to get file filename
        JsonDocument *responseDoc = web->getResponseDoc();
        responseDoc->clear(); //needed for deserializeJson?
        JsonVariant responseVariant = responseDoc->as<JsonVariant>();

        web->addResponse("pview", "file", fileName);
        web->sendDataWs(responseVariant);
        print->printJson("fixture chFun send ws done", responseVariant); //during server startup this is not send to a client, so client refresh should also trigger this
      }
    }); //fixture

    ui->initText(parentVar, "dimensions", nullptr, 32, true, [](JsonObject var) { //uiFun
      char details[32] = "";
      print->fFormat(details, sizeof(details)-1, "P:%dx%dx%d V:%dx%dx%d", ledsV.widthP, ledsV.heightP, ledsV.depthP, ledsV.widthV, ledsV.heightV, ledsV.depthV);
      web->addResponse(var["id"], "value", details);
    });

    ui->initText(parentVar, "nrOfLeds", nullptr, 32, true, [](JsonObject var) { //uiFun
      char details[32] = "";
      print->fFormat(details, sizeof(details)-1, "P:%d V:%d", ledsV.nrOfLedsP, ledsV.nrOfLedsV);
      web->addResponse(var["id"], "value", details);
      web->addResponseV(var["id"], "comment", "Max %d", NUM_LEDS_Preview);
    });

    ui->initNumber(parentVar, "fps", fps, 1, 999, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Frames per second");
    }, [this](JsonObject var, uint8_t) { //chFun
      fps = var["value"];
    });

    ui->initText(parentVar, "realFps", nullptr, 10, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Depends on how much leds fastled has configured");
    });

    #ifdef USERMOD_E131
      // if (e131mod->isEnabled) {
          e131mod->patchChannel(0, "bri", 255); //should be 256??
          e131mod->patchChannel(1, "fx", effects.size());
          e131mod->patchChannel(2, "pal", 8); //tbd: calculate nr of palettes (from select)
          // //add these temporary to test remote changing of this values do not crash the system
          // e131mod->patchChannel(3, "pro", Projections::count);
          // e131mod->patchChannel(4, "fixture", 5); //assuming 5!!!

          // ui->stageVarChanged = true;
          ui->processUiFun("e131Tbl"); //rebuild the table
      // }
      // else
      //   USER_PRINTF("Leds e131 not enabled\n");
    #endif

    effects.setup();

    FastLED.setMaxPowerInVoltsAndMilliamps(5,2000); // 5v, 2000mA

    USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // SysModule::loop();

    //set new frame
    if (millis() - frameMillis >= 1000.0/fps) {
      frameMillis = millis();

      newFrame = true;

      //for each programmed effect
      //  run the next frame of the effect

      effects.loop(ledsV.fx);

      FastLED.show();  

      frameCounter++;
    }
    else {
      newFrame = false;
    }

    JsonObject var = mdl->findVar("System");
    if (!var["canvasData"].isNull()) {
      const char * canvasData = var["canvasData"]; //0 - 494 - 140,150,0
      USER_PRINTF("AppModLeds loop canvasData %s\n", canvasData);

      char * token = strtok((char *)canvasData, ",");
      if (token != NULL) ledsV.pointX = atoi(token) / 10; else ledsV.pointX = 0; //should never happen
      token = strtok(NULL, ",");
      if (token != NULL) ledsV.pointY = atoi(token) / 10; else ledsV.pointY = 0;
      token = strtok(NULL, ",");
      if (token != NULL) ledsV.pointZ = atoi(token) / 10; else ledsV.pointZ = 0;

      mdl->setValueI("pointX", ledsV.pointX, 0);
      mdl->setValueI("pointY", ledsV.pointY, 0);
      mdl->setValueI("pointZ", ledsV.pointZ, 0);

      var.remove("canvasData");
      doMap = true; //recalc projection
    }

    //update projection
    if (millis() - lastMappingMillis >= 1000 && doMap) { //not more then once per second (for E131)
      lastMappingMillis = millis();
      doMap = false;
      ledsV.fixtureProjectAndMap();

      //https://github.com/FastLED/FastLED/wiki/Multiple-Controller-Examples

      //allocatePins
      uint8_t pinNr=0;
      for (PinObject pinObject:pins->pinObjects) {
        if (strcmp(pinObject.owner, "Leds")== 0) {
          //dirty trick to decode nrOfLedsPerPin
          char * after = strtok((char *)pinObject.details, "-");
          if (after != NULL ) {
            char * before;
            before = after;
            after = strtok(NULL, " ");
            uint16_t startLed = atoi(before);
            uint16_t nrOfLeds = atoi(after) - atoi(before) + 1;
            USER_PRINTF("FastLED.addLeds new %d: %d-%d\n", pinNr, startLed, nrOfLeds);

            //commented pins: error: static assertion failed: Invalid pin specified
            switch (pinNr) {
              case 0: FastLED.addLeds<NEOPIXEL, 0>(ledsP, startLed, nrOfLeds); break;
              case 1: FastLED.addLeds<NEOPIXEL, 1>(ledsP, startLed, nrOfLeds); break;
              case 2: FastLED.addLeds<NEOPIXEL, 2>(ledsP, startLed, nrOfLeds); break;
              case 3: FastLED.addLeds<NEOPIXEL, 3>(ledsP, startLed, nrOfLeds); break;
              case 4: FastLED.addLeds<NEOPIXEL, 4>(ledsP, startLed, nrOfLeds); break;
              case 5: FastLED.addLeds<NEOPIXEL, 5>(ledsP, startLed, nrOfLeds); break;
              // case 6: FastLED.addLeds<NEOPIXEL, 6>(ledsP, startLed, nrOfLeds); break;
              // case 7: FastLED.addLeds<NEOPIXEL, 7>(ledsP, startLed, nrOfLeds); break;
              // case 8: FastLED.addLeds<NEOPIXEL, 8>(ledsP, startLed, nrOfLeds); break;
              // case 9: FastLED.addLeds<NEOPIXEL, 9>(ledsP, startLed, nrOfLeds); break;
              // case 10: FastLED.addLeds<NEOPIXEL, 10>(ledsP, startLed, nrOfLeds); break;
              case 11: FastLED.addLeds<NEOPIXEL, 11>(ledsP, startLed, nrOfLeds); break;
              case 12: FastLED.addLeds<NEOPIXEL, 12>(ledsP, startLed, nrOfLeds); break;
              case 13: FastLED.addLeds<NEOPIXEL, 13>(ledsP, startLed, nrOfLeds); break;
              case 14: FastLED.addLeds<NEOPIXEL, 14>(ledsP, startLed, nrOfLeds); break;
              case 15: FastLED.addLeds<NEOPIXEL, 15>(ledsP, startLed, nrOfLeds); break;
              case 16: FastLED.addLeds<NEOPIXEL, 16>(ledsP, startLed, nrOfLeds); break;
              case 17: FastLED.addLeds<NEOPIXEL, 17>(ledsP, startLed, nrOfLeds); break;
              case 18: FastLED.addLeds<NEOPIXEL, 18>(ledsP, startLed, nrOfLeds); break;
              case 19: FastLED.addLeds<NEOPIXEL, 19>(ledsP, startLed, nrOfLeds); break;
              // case 20: FastLED.addLeds<NEOPIXEL, 20>(ledsP, startLed, nrOfLeds); break;
              case 21: FastLED.addLeds<NEOPIXEL, 21>(ledsP, startLed, nrOfLeds); break;
              case 22: FastLED.addLeds<NEOPIXEL, 22>(ledsP, startLed, nrOfLeds); break;
              case 23: FastLED.addLeds<NEOPIXEL, 23>(ledsP, startLed, nrOfLeds); break;
              // case 24: FastLED.addLeds<NEOPIXEL, 24>(ledsP, startLed, nrOfLeds); break;
              case 25: FastLED.addLeds<NEOPIXEL, 25>(ledsP, startLed, nrOfLeds); break;
              case 26: FastLED.addLeds<NEOPIXEL, 26>(ledsP, startLed, nrOfLeds); break;
              case 27: FastLED.addLeds<NEOPIXEL, 27>(ledsP, startLed, nrOfLeds); break;
              // case 28: FastLED.addLeds<NEOPIXEL, 28>(ledsP, startLed, nrOfLeds); break;
              // case 29: FastLED.addLeds<NEOPIXEL, 29>(ledsP, startLed, nrOfLeds); break;
              // case 30: FastLED.addLeds<NEOPIXEL, 30>(ledsP, startLed, nrOfLeds); break;
              // case 31: FastLED.addLeds<NEOPIXEL, 31>(ledsP, startLed, nrOfLeds); break;
              case 32: FastLED.addLeds<NEOPIXEL, 32>(ledsP, startLed, nrOfLeds); break;
              case 33: FastLED.addLeds<NEOPIXEL, 33>(ledsP, startLed, nrOfLeds); break;
              // case 34: FastLED.addLeds<NEOPIXEL, 34>(ledsP, startLed, nrOfLeds); break;
              // case 35: FastLED.addLeds<NEOPIXEL, 35>(ledsP, startLed, nrOfLeds); break;
              // case 36: FastLED.addLeds<NEOPIXEL, 36>(ledsP, startLed, nrOfLeds); break;
              // case 37: FastLED.addLeds<NEOPIXEL, 37>(ledsP, startLed, nrOfLeds); break;
              // case 38: FastLED.addLeds<NEOPIXEL, 38>(ledsP, startLed, nrOfLeds); break;
              // case 39: FastLED.addLeds<NEOPIXEL, 39>(ledsP, startLed, nrOfLeds); break;
              // case 40: FastLED.addLeds<NEOPIXEL, 40>(ledsP, startLed, nrOfLeds); break;
              // case 41: FastLED.addLeds<NEOPIXEL, 41>(ledsP, startLed, nrOfLeds); break;
              // case 42: FastLED.addLeds<NEOPIXEL, 42>(ledsP, startLed, nrOfLeds); break;
              // case 43: FastLED.addLeds<NEOPIXEL, 43>(ledsP, startLed, nrOfLeds); break;
              // case 44: FastLED.addLeds<NEOPIXEL, 44>(ledsP, startLed, nrOfLeds); break;
              // case 45: FastLED.addLeds<NEOPIXEL, 45>(ledsP, startLed, nrOfLeds); break;
              // case 46: FastLED.addLeds<NEOPIXEL, 46>(ledsP, startLed, nrOfLeds); break;
              // case 47: FastLED.addLeds<NEOPIXEL, 47>(ledsP, startLed, nrOfLeds); break;
              // case 48: FastLED.addLeds<NEOPIXEL, 48>(ledsP, startLed, nrOfLeds); break;
              // case 49: FastLED.addLeds<NEOPIXEL, 49>(ledsP, startLed, nrOfLeds); break;
              // case 50: FastLED.addLeds<NEOPIXEL, 50>(ledsP, startLed, nrOfLeds); break;
              default: USER_PRINTF("FastLedPin assignment: pin not supported %d\n", pinNr);
            }
          }
        }
        pinNr++;
      }
    }
  } //loop

  void loop1s() {
    mdl->setValueLossy("realFps", "%lu /s", frameCounter);
    frameCounter = 0;
  }

private:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;

};

static AppModLeds *lds;