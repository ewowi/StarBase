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

#include "AppLeds.h"
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

  Leds leds = Leds(); //virtual leds
  Fixture fixture = Fixture();


  AppModLeds() :SysModule("Leds") {
    fixture.leds = &leds;
    leds.fixture = &fixture;
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name);
    if (parentVar["o"] > -1000) parentVar["o"] = -1100; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

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
    currentVar["log"] = true; //logarithmic: not needed when using FastLED setCorrection
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
      doMap = effects.setEffect(leds, var, rowNr);
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

    }, [this](JsonObject var, uint8_t rowNr) { //chFun

      leds.projectionNr = mdl->getValue(var, rowNr);
      doMap = true;

    });
    currentVar["stage"] = true;

    ui->initCoord3D(tableVar, "fxStart", leds.startPos, 0, UINT16_MAX, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Start");
    }, [this](JsonObject var, uint8_t rowNr) { //chFun

      leds.startPos = mdl->getValue(var, rowNr).as<Coord3D>();

      USER_PRINTF("fxStart %d %d %d - %d %d %d\n", leds.startPos.x, leds.startPos.y, leds.startPos.z, leds.endPos.x, leds.endPos.y, leds.endPos.z);

      leds.fadeToBlackBy();

      doMap = true;
    });

    ui->initCoord3D(tableVar, "fxEnd", leds.endPos, 0, UINT16_MAX, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "End");
    }, [this](JsonObject var, uint8_t rowNr) { //chFun

      leds.endPos = mdl->getValue(var, rowNr).as<Coord3D>();

      USER_PRINTF("fxEnd chFun %d %d %d - %d %d %d\n", leds.startPos.x, leds.startPos.y, leds.startPos.z, leds.endPos.x, leds.endPos.y, leds.endPos.z);

      leds.fadeToBlackBy();

      doMap = true;
    });

    ui->initCoord3D(tableVar, "fxSize", leds.size, 0, UINT16_MAX, true, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Size");
    });

    ui->initNumber(tableVar, "fxCount", leds.nrOfLeds, 0, UINT16_MAX, true, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Count");
    });

    #ifdef USERMOD_WLEDAUDIO
      ui->initCheckBox(parentVar, "mHead", false, false, [](JsonObject var) { //uiFun
        web->addResponse(var["id"], "label", "Moving heads");
        web->addResponse(var["id"], "comment", "Move on GEQ");
      });
    #endif

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
  }

  void loop() {
    // SysModule::loop();

    //set new frame
    if (millis() - frameMillis >= 1000.0/fps) {
      frameMillis = millis();

      newFrame = true;

      //for each programmed effect
      //  run the next frame of the effect

      effects.loop(leds);

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

      leds.fadeToBlackBy();

      char * token = strtok((char *)canvasData, ":");
      bool isStart = strcmp(token, "start") == 0;

      Coord3D *startOrEndPos = isStart? &leds.startPos: &leds.endPos;

      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->x = atoi(token) / 10; else startOrEndPos->x = 0; //should never happen
      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->y = atoi(token) / 10; else startOrEndPos->y = 0;
      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->z = atoi(token) / 10; else startOrEndPos->z = 0;

      mdl->setValue(isStart?"fxStart":"fxEnd", *startOrEndPos, 0); //assuming row 0 for the moment

      var.remove("canvasData"); //convasdata has been processed
      doMap = true; //recalc projection
    }

    //update projection
    if (millis() - lastMappingMillis >= 1000 && doMap) { //not more then once per second (for E131)
      lastMappingMillis = millis();
      doMap = false;
      leds.fixtureProjectAndMap();

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
                case 0: FastLED.addLeds<NEOPIXEL, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 1: FastLED.addLeds<NEOPIXEL, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 2: FastLED.addLeds<NEOPIXEL, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 3: FastLED.addLeds<NEOPIXEL, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 4: FastLED.addLeds<NEOPIXEL, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 5: FastLED.addLeds<NEOPIXEL, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 6: FastLED.addLeds<NEOPIXEL, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 7: FastLED.addLeds<NEOPIXEL, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 8: FastLED.addLeds<NEOPIXEL, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 9: FastLED.addLeds<NEOPIXEL, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 10: FastLED.addLeds<NEOPIXEL, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 11: FastLED.addLeds<NEOPIXEL, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 12: FastLED.addLeds<NEOPIXEL, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 13: FastLED.addLeds<NEOPIXEL, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 14: FastLED.addLeds<NEOPIXEL, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 15: FastLED.addLeds<NEOPIXEL, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 16: FastLED.addLeds<NEOPIXEL, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 17: FastLED.addLeds<NEOPIXEL, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 18: FastLED.addLeds<NEOPIXEL, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 19: FastLED.addLeds<NEOPIXEL, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 20: FastLED.addLeds<NEOPIXEL, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 21: FastLED.addLeds<NEOPIXEL, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 22: FastLED.addLeds<NEOPIXEL, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 23: FastLED.addLeds<NEOPIXEL, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 24: FastLED.addLeds<NEOPIXEL, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 25: FastLED.addLeds<NEOPIXEL, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 26: FastLED.addLeds<NEOPIXEL, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 27: FastLED.addLeds<NEOPIXEL, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 28: FastLED.addLeds<NEOPIXEL, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 29: FastLED.addLeds<NEOPIXEL, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 30: FastLED.addLeds<NEOPIXEL, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 31: FastLED.addLeds<NEOPIXEL, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 32: FastLED.addLeds<NEOPIXEL, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 33: FastLED.addLeds<NEOPIXEL, 33>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 34: FastLED.addLeds<NEOPIXEL, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 35: FastLED.addLeds<NEOPIXEL, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 36: FastLED.addLeds<NEOPIXEL, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 37: FastLED.addLeds<NEOPIXEL, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 38: FastLED.addLeds<NEOPIXEL, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 39: FastLED.addLeds<NEOPIXEL, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 40: FastLED.addLeds<NEOPIXEL, 40>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 41: FastLED.addLeds<NEOPIXEL, 41>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 42: FastLED.addLeds<NEOPIXEL, 42>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 43: FastLED.addLeds<NEOPIXEL, 43>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 44: FastLED.addLeds<NEOPIXEL, 44>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 45: FastLED.addLeds<NEOPIXEL, 45>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 46: FastLED.addLeds<NEOPIXEL, 46>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 47: FastLED.addLeds<NEOPIXEL, 47>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 48: FastLED.addLeds<NEOPIXEL, 48>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 49: FastLED.addLeds<NEOPIXEL, 49>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 50: FastLED.addLeds<NEOPIXEL, 50>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif //CONFIG_IDF_TARGET_ESP32

              #if CONFIG_IDF_TARGET_ESP32S2
                case 0: FastLED.addLeds<NEOPIXEL, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 1: FastLED.addLeds<NEOPIXEL, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 2: FastLED.addLeds<NEOPIXEL, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 3: FastLED.addLeds<NEOPIXEL, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 4: FastLED.addLeds<NEOPIXEL, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 5: FastLED.addLeds<NEOPIXEL, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 6: FastLED.addLeds<NEOPIXEL, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 7: FastLED.addLeds<NEOPIXEL, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 8: FastLED.addLeds<NEOPIXEL, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 9: FastLED.addLeds<NEOPIXEL, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 10: FastLED.addLeds<NEOPIXEL, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 11: FastLED.addLeds<NEOPIXEL, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 12: FastLED.addLeds<NEOPIXEL, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 13: FastLED.addLeds<NEOPIXEL, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 14: FastLED.addLeds<NEOPIXEL, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 15: FastLED.addLeds<NEOPIXEL, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 16: FastLED.addLeds<NEOPIXEL, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 17: FastLED.addLeds<NEOPIXEL, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 18: FastLED.addLeds<NEOPIXEL, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 19: FastLED.addLeds<NEOPIXEL, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 20: FastLED.addLeds<NEOPIXEL, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 21: FastLED.addLeds<NEOPIXEL, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 22: FastLED.addLeds<NEOPIXEL, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 23: FastLED.addLeds<NEOPIXEL, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 24: FastLED.addLeds<NEOPIXEL, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 25: FastLED.addLeds<NEOPIXEL, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 26: FastLED.addLeds<NEOPIXEL, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 27: FastLED.addLeds<NEOPIXEL, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 28: FastLED.addLeds<NEOPIXEL, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 29: FastLED.addLeds<NEOPIXEL, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 30: FastLED.addLeds<NEOPIXEL, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 31: FastLED.addLeds<NEOPIXEL, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 32: FastLED.addLeds<NEOPIXEL, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 33: FastLED.addLeds<NEOPIXEL, 33>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 34: FastLED.addLeds<NEOPIXEL, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 35: FastLED.addLeds<NEOPIXEL, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 36: FastLED.addLeds<NEOPIXEL, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 37: FastLED.addLeds<NEOPIXEL, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 38: FastLED.addLeds<NEOPIXEL, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 39: FastLED.addLeds<NEOPIXEL, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 40: FastLED.addLeds<NEOPIXEL, 40>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 41: FastLED.addLeds<NEOPIXEL, 41>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 42: FastLED.addLeds<NEOPIXEL, 42>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 43: FastLED.addLeds<NEOPIXEL, 43>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 44: FastLED.addLeds<NEOPIXEL, 44>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 45: FastLED.addLeds<NEOPIXEL, 45>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 46: FastLED.addLeds<NEOPIXEL, 46>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 47: FastLED.addLeds<NEOPIXEL, 47>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 48: FastLED.addLeds<NEOPIXEL, 48>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 49: FastLED.addLeds<NEOPIXEL, 49>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 50: FastLED.addLeds<NEOPIXEL, 50>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 51: FastLED.addLeds<NEOPIXEL, 51>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
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
    mdl->setValueUIOnly("realFps", "%lu /s", frameCounter);
    frameCounter = 0;
  }

private:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;

};

static AppModLeds *lds;