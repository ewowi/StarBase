/*
   @title     StarMod
   @file      AppModLeds.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

#include "AppLedsV.h"
#include "AppEffects.h"
#ifdef USERMOD_E131
  #include "../User/UserModE131.h"
#endif

#include <vector>
// FastLED optional flags to configure drivers, see https://github.com/FastLED/FastLED/blob/master/src/platforms/esp/32
// RMT driver (default)
// #define FASTLED_ESP32_FLASH_LOCK 1    // temporarily disabled FLASH file access while driving LEDs (may prevent random flicker)
// #define FASTLED_RMT_BUILTIN_DRIVER 1  // in case your app needs to use RMT units, too (slower)
// I2S parallel driver
// #define FASTLED_ESP32_I2S true        // to use I2S parallel driver (instead of RMT)
// #define I2S_DEVICE 1                  // I2S driver: allows to still use I2S#0 for audio (only on esp32 and esp32-s3)
// #define FASTLED_I2S_MAX_CONTROLLERS 8 // 8 LED pins should be enough (default = 24)
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

    parentVar = ui->initAppMod(parentVar, name);
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

    JsonObject tableVar = ui->initTable(parentVar, "fxTbl", nullptr, false, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Effects");
      web->addResponse(var["id"], "comment", "List of effects (WIP)");
      JsonArray rows = web->addResponseA(var["id"], "value"); //overwrite the value

      //add value for each child
      JsonArray row = rows.createNestedArray(); //one row of data
      for (JsonObject childVar : var["n"].as<JsonArray>()) {
        // print->printJson("fxTbl childs", childVar);
        row.add(childVar["value"]);
        //recursive
        // for (JsonObject childVar : childVar["n"].as<JsonArray>()) {
        //   print->printJson("fxTbl child childs", childVar);
        //   row.add(childVar["value"]);
        // }
      }

      // print->printJson("fxTbl values", rows);
    });

    currentVar = ui->initSelect(parentVar, "fx", 0, false, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Effect");
      web->addResponse(var["id"], "comment", "Effect to show");
      JsonArray select = web->addResponseA(var["id"], "options");
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
      JsonArray select = web->addResponseA(var["id"], "options");
      select.add("None"); // 0
      select.add("Random"); // 1
      select.add("Distance from point"); //2
      select.add("Distance from center"); //3
      select.add("Mirror"); //4
      select.add("Reverse"); //5
      select.add("Multiply"); //6
      select.add("Kaleidoscope"); //7
      select.add("Fun"); //8

      //set default value (array)
      // if (!var["value"].isNull()) {
        // JsonArray value = web->addResponseA(var["id"], "value");
        // value.add(1);
        // value.add(2);
      // }
    }, [this](JsonObject var, uint8_t rowNr) { //chFun

      ledsV.projectionNr = mdl->getValue(var, rowNr);
      doMap = true;

    });
    currentVar["stage"] = true;

    ui->initCoord3D(tableVar, "fxStart", 0, 0, 127, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Start");
    }, [this](JsonObject var, uint8_t rowNr) { //chFun

      ledsV.fadeToBlackBy();

      ledsV.startPos = mdl->getValue(var, rowNr).as<Coord3D>();
      doMap = true;
    });
    ui->initCoord3D(tableVar, "fxEnd", 0, 0, 127, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "End");
    }, [this](JsonObject var, uint8_t rowNr) { //chFun

      ledsV.fadeToBlackBy();

      ledsV.endPos = mdl->getValue(var, rowNr).as<Coord3D>();
      doMap = true;
    });

    ui->initSelect(parentVar, "fixture", 0, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Fixture to display effect on");
      JsonArray select = web->addResponseA(var["id"], "options");
      files->dirToJson(select, true, "D"); //only files containing D (1D,2D,3D), alphabetically, only looking for D not very destinctive though

      // ui needs to load the file also initially
      char fileName[32] = "";
      if (files->seqNrToName(fileName, var["value"])) {
        web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
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

        web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
        web->sendDataWs(responseVariant);
        print->printJson("fixture chFun send ws done", responseVariant); //during server startup this is not send to a client, so client refresh should also trigger this
      }
    }); //fixture

    ui->initText(tableVar, "dimensions", nullptr, 32, true, [](JsonObject var) { //uiFun
      char details[32] = "";
      print->fFormat(details, sizeof(details)-1, "P:%dx%dx%d V:%dx%dx%d", ledsV.widthP, ledsV.heightP, ledsV.depthP, ledsV.widthV, ledsV.heightV, ledsV.depthV);
      web->addResponse(var["id"], "value", JsonString(details, JsonString::Copied));
    });

    ui->initText(tableVar, "nrOfLeds", nullptr, 32, true, [](JsonObject var) { //uiFun
      char details[32] = "";
      print->fFormat(details, sizeof(details)-1, "P:%d V:%d", ledsV.nrOfLedsP, ledsV.nrOfLedsV);
      web->addResponse(var["id"], "value", JsonString(details, JsonString::Copied));
      web->addResponseV(var["id"], "comment", "Max %d", NUM_LEDS_Max);
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

      ledsV.fadeToBlackBy();

      char * token = strtok((char *)canvasData, ":");
      bool isStart = strcmp(token, "start") == 0;

      Coord3D *startOrEndPos = isStart? &ledsV.startPos: &ledsV.endPos;

      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->x = atoi(token) / 10; else startOrEndPos->x = 0; //should never happen
      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->y = atoi(token) / 10; else startOrEndPos->y = 0;
      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->z = atoi(token) / 10; else startOrEndPos->z = 0;

      mdl->setValue(isStart?"fxStart":"fxEnd", *startOrEndPos, 0);

      var.remove("canvasData"); //convasdata has been processed
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
              #if CONFIG_IDF_TARGET_ESP32
                case 0: FastLED.addLeds<NEOPIXEL, 0>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 1: FastLED.addLeds<NEOPIXEL, 1>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 2: FastLED.addLeds<NEOPIXEL, 2>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 3: FastLED.addLeds<NEOPIXEL, 3>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 4: FastLED.addLeds<NEOPIXEL, 4>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 5: FastLED.addLeds<NEOPIXEL, 5>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 6: FastLED.addLeds<NEOPIXEL, 6>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 7: FastLED.addLeds<NEOPIXEL, 7>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 8: FastLED.addLeds<NEOPIXEL, 8>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 9: FastLED.addLeds<NEOPIXEL, 9>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 10: FastLED.addLeds<NEOPIXEL, 10>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 11: FastLED.addLeds<NEOPIXEL, 11>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 12: FastLED.addLeds<NEOPIXEL, 12>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 13: FastLED.addLeds<NEOPIXEL, 13>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 14: FastLED.addLeds<NEOPIXEL, 14>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 15: FastLED.addLeds<NEOPIXEL, 15>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 16: FastLED.addLeds<NEOPIXEL, 16>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 17: FastLED.addLeds<NEOPIXEL, 17>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 18: FastLED.addLeds<NEOPIXEL, 18>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 19: FastLED.addLeds<NEOPIXEL, 19>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 20: FastLED.addLeds<NEOPIXEL, 20>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 21: FastLED.addLeds<NEOPIXEL, 21>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 22: FastLED.addLeds<NEOPIXEL, 22>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 23: FastLED.addLeds<NEOPIXEL, 23>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 24: FastLED.addLeds<NEOPIXEL, 24>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 25: FastLED.addLeds<NEOPIXEL, 25>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 26: FastLED.addLeds<NEOPIXEL, 26>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 27: FastLED.addLeds<NEOPIXEL, 27>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 28: FastLED.addLeds<NEOPIXEL, 28>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 29: FastLED.addLeds<NEOPIXEL, 29>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 30: FastLED.addLeds<NEOPIXEL, 30>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 31: FastLED.addLeds<NEOPIXEL, 31>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 32: FastLED.addLeds<NEOPIXEL, 32>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 33: FastLED.addLeds<NEOPIXEL, 33>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 34: FastLED.addLeds<NEOPIXEL, 34>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 35: FastLED.addLeds<NEOPIXEL, 35>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 36: FastLED.addLeds<NEOPIXEL, 36>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 37: FastLED.addLeds<NEOPIXEL, 37>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 38: FastLED.addLeds<NEOPIXEL, 38>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 39: FastLED.addLeds<NEOPIXEL, 39>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 40: FastLED.addLeds<NEOPIXEL, 40>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 41: FastLED.addLeds<NEOPIXEL, 41>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 42: FastLED.addLeds<NEOPIXEL, 42>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 43: FastLED.addLeds<NEOPIXEL, 43>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 44: FastLED.addLeds<NEOPIXEL, 44>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 45: FastLED.addLeds<NEOPIXEL, 45>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 46: FastLED.addLeds<NEOPIXEL, 46>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 47: FastLED.addLeds<NEOPIXEL, 47>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 48: FastLED.addLeds<NEOPIXEL, 48>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 49: FastLED.addLeds<NEOPIXEL, 49>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 50: FastLED.addLeds<NEOPIXEL, 50>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
              #endif //CONFIG_IDF_TARGET_ESP32

              #if CONFIG_IDF_TARGET_ESP32S2
                case 0: FastLED.addLeds<NEOPIXEL, 0>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 1: FastLED.addLeds<NEOPIXEL, 1>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 2: FastLED.addLeds<NEOPIXEL, 2>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 3: FastLED.addLeds<NEOPIXEL, 3>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 4: FastLED.addLeds<NEOPIXEL, 4>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 5: FastLED.addLeds<NEOPIXEL, 5>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 6: FastLED.addLeds<NEOPIXEL, 6>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 7: FastLED.addLeds<NEOPIXEL, 7>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 8: FastLED.addLeds<NEOPIXEL, 8>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 9: FastLED.addLeds<NEOPIXEL, 9>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 10: FastLED.addLeds<NEOPIXEL, 10>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 11: FastLED.addLeds<NEOPIXEL, 11>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 12: FastLED.addLeds<NEOPIXEL, 12>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 13: FastLED.addLeds<NEOPIXEL, 13>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 14: FastLED.addLeds<NEOPIXEL, 14>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 15: FastLED.addLeds<NEOPIXEL, 15>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 16: FastLED.addLeds<NEOPIXEL, 16>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 17: FastLED.addLeds<NEOPIXEL, 17>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 18: FastLED.addLeds<NEOPIXEL, 18>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 19: FastLED.addLeds<NEOPIXEL, 19>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 20: FastLED.addLeds<NEOPIXEL, 20>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 21: FastLED.addLeds<NEOPIXEL, 21>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 22: FastLED.addLeds<NEOPIXEL, 22>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 23: FastLED.addLeds<NEOPIXEL, 23>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 24: FastLED.addLeds<NEOPIXEL, 24>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 25: FastLED.addLeds<NEOPIXEL, 25>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 26: FastLED.addLeds<NEOPIXEL, 26>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 27: FastLED.addLeds<NEOPIXEL, 27>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 28: FastLED.addLeds<NEOPIXEL, 28>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 29: FastLED.addLeds<NEOPIXEL, 29>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 30: FastLED.addLeds<NEOPIXEL, 30>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 31: FastLED.addLeds<NEOPIXEL, 31>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 32: FastLED.addLeds<NEOPIXEL, 32>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 33: FastLED.addLeds<NEOPIXEL, 33>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 34: FastLED.addLeds<NEOPIXEL, 34>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 35: FastLED.addLeds<NEOPIXEL, 35>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 36: FastLED.addLeds<NEOPIXEL, 36>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 37: FastLED.addLeds<NEOPIXEL, 37>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 38: FastLED.addLeds<NEOPIXEL, 38>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 39: FastLED.addLeds<NEOPIXEL, 39>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 40: FastLED.addLeds<NEOPIXEL, 40>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 41: FastLED.addLeds<NEOPIXEL, 41>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 42: FastLED.addLeds<NEOPIXEL, 42>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 43: FastLED.addLeds<NEOPIXEL, 43>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 44: FastLED.addLeds<NEOPIXEL, 44>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                case 45: FastLED.addLeds<NEOPIXEL, 45>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 46: FastLED.addLeds<NEOPIXEL, 46>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 47: FastLED.addLeds<NEOPIXEL, 47>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 48: FastLED.addLeds<NEOPIXEL, 48>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 49: FastLED.addLeds<NEOPIXEL, 49>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 50: FastLED.addLeds<NEOPIXEL, 50>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
                // case 51: FastLED.addLeds<NEOPIXEL, 51>(ledsV.ledsPhysical, startLed, nrOfLeds); break;
              #endif //CONFIG_IDF_TARGET_ESP32S2

              default: USER_PRINTF("FastLedPin assignment: pin not supported %d\n", pinNr);
            }
          }
        }
        pinNr++;
      }
    }
  } //loop

  void loop1s() {
    mdl->setValueV("realFps", "%lu /s", frameCounter);
    frameCounter = 0;
  }

private:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;

};

static AppModLeds *lds;