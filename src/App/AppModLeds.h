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

// FastLED optional flags to configure drivers, see https://github.com/FastLED/FastLED/blob/master/src/platforms/esp/32
// RMT driver (default)
// #define FASTLED_ESP32_FLASH_LOCK 1    // temporarily disabled FLASH file access while driving LEDs (may prevent random flicker)
// #define FASTLED_RMT_BUILTIN_DRIVER 1  // in case your app needs to use RMT units, too (slower)
// I2S parallel driver
// #define FASTLED_ESP32_I2S true        // to use I2S parallel driver (instead of RMT)
// #define I2S_DEVICE 1                  // I2S driver: allows to still use I2S#0 for audio (only on esp32 and esp32-s3)
// #define FASTLED_I2S_MAX_CONTROLLERS 8 // 8 LED pins should be enough (default = 24)
#include "FastLED.h"

#include "AppFixture.h"
#include "AppEffects.h"

// #ifdef STARMOD_USERMOD_E131
//   #include "../User/UserModE131.h"
// #endif

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
  Effects effects;

  Fixture fixture = Fixture();

  AppModLeds() :SysModule("Leds") {
    fixture.ledsList.push_back(Leds(fixture.ledsList.size(), fixture));
    fixture.ledsList.push_back(Leds(fixture.ledsList.size(), fixture));
    USER_PRINTF("Leds created\n");
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name);
    if (parentVar["o"] > -1000) parentVar["o"] = -1100; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

    JsonObject currentVar;

    JsonObject tableVar = ui->initTable(parentVar, "fxTbl", nullptr, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Effects");
        ui->setComment(var, "List of effects");
        return true;
      case f_AddRow: {
        rowNr = fixture.ledsList.size();
        USER_PRINTF("chFun addRow %s[%d]\n", mdl->varID(var), rowNr);

        web->getResponseObject()["addRow"]["rowNr"] = rowNr;

        if (rowNr >= fixture.ledsList.size())
          fixture.ledsList.push_back(Leds(fixture.ledsList.size(), fixture));
        return true;
      }
      case f_DelRow: {
        USER_PRINTF("chFun delrow %s[%d]\n", mdl->varID(var), rowNr);
        //tbd: fade to black
        if (rowNr <fixture.ledsList.size()) {
          fixture.ledsList.erase(fixture.ledsList.begin() + rowNr); //remove from leds
        }
        return true;
      }
      default: return false;
    }});

    currentVar = ui->initSelect(tableVar, "fx", 0, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNr = 0; rowNr < fixture.ledsList.size(); rowNr++)
          mdl->setValue(var, fixture.ledsList[rowNr].fx, rowNr);
        return true;
      case f_UIFun: {
        ui->setLabel(var, "Effect");
        ui->setComment(var, "Effect to show");
        JsonArray options = ui->setOptions(var);
        for (Effect *effect:effects.effects) {
          options.add(effect->name());
        }
        return true;
      }
      case f_ChangeFun:
        if (rowNr < fixture.ledsList.size()) {
          effects.setEffect(fixture.ledsList[rowNr], var, rowNr);

          web->addResponse("details", "var", var);
          web->addResponse("details", "rowNr", rowNr);
        }
        return true;
      default: return false;
    }});
    currentVar["stage"] = true;

    currentVar = ui->initSelect(tableVar, "pro", 2, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNr = 0; rowNr < fixture.ledsList.size(); rowNr++)
          mdl->setValue(var, fixture.ledsList[rowNr].projectionNr, rowNr);
        return true;
      case f_UIFun: {
        ui->setLabel(var, "Projection");
        ui->setComment(var, "How to project fx");
        JsonArray options = ui->setOptions(var);
        options.add("None"); // 0
        options.add("Random"); // 1
        options.add("Distance from point"); //2
        options.add("Distance from center"); //3
        options.add("Mirror"); //4
        options.add("Reverse"); //5
        options.add("Multiply"); //6
        options.add("Kaleidoscope"); //7
        options.add("Fun"); //8
        return true;
      }
      case f_ChangeFun:
        if (rowNr < fixture.ledsList.size()) {
          fixture.ledsList[rowNr].projectionNr = mdl->getValue(var, rowNr);
          USER_PRINTF("chFun pro[%d] <- %d (%d)\n", rowNr, fixture.ledsList[rowNr].projectionNr, fixture.ledsList.size());

          fixture.ledsList[rowNr].doMap = true;
          fixture.doMap = true;
        }
        return true;
      default: return false;
    }});
    currentVar["stage"] = true;

    ui->initCoord3D(tableVar, "fxStart", fixture.ledsList[0].startPos, 0, NUM_LEDS_Max, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        USER_PRINTF("fxStart[%d] valueFun %d,%d,%d\n", rowNr, fixture.ledsList[rowNr].startPos.x, fixture.ledsList[rowNr].startPos.y, fixture.ledsList[rowNr].startPos.z);
        for (uint8_t rowNr = 0; rowNr < fixture.ledsList.size(); rowNr++)
          mdl->setValue(var, fixture.ledsList[rowNr].startPos, rowNr);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Start");
        return true;
      case f_ChangeFun:
        if (rowNr < fixture.ledsList.size()) {
          fixture.ledsList[rowNr].startPos = mdl->getValue(var, rowNr).as<Coord3D>();

          USER_PRINTF("fxStart[%d] chFun %d,%d,%d\n", rowNr, fixture.ledsList[rowNr].startPos.x, fixture.ledsList[rowNr].startPos.y, fixture.ledsList[rowNr].startPos.z);

          fixture.ledsList[rowNr].fadeToBlackBy();
          fixture.ledsList[rowNr].doMap = true;
          fixture.doMap = true;
        }
        else {
          USER_PRINTF("fxStart[%d] chfun rownr not in range > %d\n", rowNr, fixture.ledsList.size());
        }
        return true;
      default: return false;
    }});

    ui->initCoord3D(tableVar, "fxEnd", fixture.ledsList[0].endPos, 0, NUM_LEDS_Max, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNr = 0; rowNr < fixture.ledsList.size(); rowNr++)
          mdl->setValue(var, fixture.ledsList[rowNr].endPos, rowNr);
        USER_PRINTF("fxEnd[%d] valueFun %d,%d,%d\n", rowNr, fixture.ledsList[rowNr].endPos.x, fixture.ledsList[rowNr].endPos.y, fixture.ledsList[rowNr].endPos.z);
        return true;
      case f_UIFun:
        ui->setLabel(var, "End");
        return true;
      case f_ChangeFun:
        if (rowNr < fixture.ledsList.size()) {
          fixture.ledsList[rowNr].endPos = mdl->getValue(var, rowNr).as<Coord3D>();

          USER_PRINTF("fxEnd[%d] chFun %d,%d,%d\n", rowNr, fixture.ledsList[rowNr].endPos.x, fixture.ledsList[rowNr].endPos.y, fixture.ledsList[rowNr].endPos.z);

          fixture.ledsList[rowNr].fadeToBlackBy();
          fixture.ledsList[rowNr].doMap = true;
          fixture.doMap = true;
        }
        else {
          USER_PRINTF("fxEnd[%d] chfun rownr not in range > %d\n", rowNr, fixture.ledsList.size());
        }
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "fxSize", nullptr, 32, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (std::vector<Leds>::iterator leds=fixture.ledsList.begin(); leds!=fixture.ledsList.end(); ++leds) {
          char message[32];
          print->fFormat(message, sizeof(message)-1, "%d x %d x %d = %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          USER_PRINTF("fxSize[%d]([%d 0f %d]) = %s\n", leds - fixture.ledsList.begin(), fixture.ledsList[leds - fixture.ledsList.begin()].rowNr, fixture.ledsList.size(), message);
          mdl->setValue(var, JsonString(message, JsonString::Copied), leds - fixture.ledsList.begin()); //rowNr
        }
        return true;
      case f_UIFun:
        ui->setLabel(var, "Size");
        return true;
      default: return false;
    }});

    ui->initSelect(parentVar, "fxLayout", 0, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Layout");
        ui->setComment(var, "WIP");
        JsonArray options = ui->setOptions(var);
        options.add("□"); //0
        options.add("="); //1
        options.add("||"); //2
        options.add("+"); //3
        return true;
      }
      default: return false;
    }}); //fixtureGen

    ui->initSlider(parentVar, "Blending", fixture.globalBlend, 0, 255, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ChangeFun:
        fixture.globalBlend = var["value"];
        return true;
      default: return false;
    }});

    #ifdef STARMOD_USERMOD_E131
      // if (e131mod->isEnabled) {
          e131mod->patchChannel(0, "bri", 255); //should be 256??
          e131mod->patchChannel(1, "fx", effects.effects.size());
          e131mod->patchChannel(2, "pal", 8); //tbd: calculate nr of palettes (from select)
          // //add these temporary to test remote changing of this values do not crash the system
          // e131mod->patchChannel(3, "pro", Projections::count);
          // e131mod->patchChannel(4, "fixture", 5); //assuming 5!!!

          // ui->stageVarChanged = true;
          // //rebuild the table
          for (JsonObject childVar: mdl->varN("e131Tbl"))
            ui->callVarFun(childVar, UINT8_MAX, f_ValueFun);

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
      // vector iteration on classes is faster!!! (22 vs 30 fps !!!!)
      for (std::vector<Leds>::iterator leds=fixture.ledsList.begin(); leds!=fixture.ledsList.end(); ++leds) {
        // USER_PRINTF(" %d %d,%d,%d - %d,%d,%d (%d,%d,%d)", leds->fx, leds->startPos.x, leds->startPos.y, leds->startPos.z, leds->endPos.x, leds->endPos.y, leds->endPos.z, leds->size.x, leds->size.y, leds->size.z );
        effects.loop(*leds);
      }

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

      //currently only leds[0] supported

      fixture.ledsList[0].fadeToBlackBy();

      char * token = strtok((char *)canvasData, ":");
      bool isStart = strcmp(token, "start") == 0;

      Coord3D *startOrEndPos = isStart? &fixture.ledsList[0].startPos: &fixture.ledsList[0].endPos;

      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->x = atoi(token) / 10; else startOrEndPos->x = 0; //should never happen
      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->y = atoi(token) / 10; else startOrEndPos->y = 0;
      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->z = atoi(token) / 10; else startOrEndPos->z = 0;

      mdl->setValue(isStart?"fxStart":"fxEnd", *startOrEndPos, 0); //assuming row 0 for the moment

      var.remove("canvasData"); //convasdata has been processed
      fixture.ledsList[0].doMap = true; //recalc projection
      fixture.doMap = true;
    }

    //update projection
    if (millis() - lastMappingMillis >= 1000 && fixture.doMap) { //not more then once per second (for E131)
      lastMappingMillis = millis();
      fixture.projectAndMap();

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
            #if !defined(BOARD_HAS_PSRAM) && !defined(ARDUINO_ESP32_PICO)
                // 16+17 = reserved for PSRAM, or reserved for FLASH on pico-D4
                case 16: FastLED.addLeds<NEOPIXEL, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 17: FastLED.addLeds<NEOPIXEL, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
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
                // 34-39 input-only
                // case 34: FastLED.addLeds<NEOPIXEL, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 35: FastLED.addLeds<NEOPIXEL, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 36: FastLED.addLeds<NEOPIXEL, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 37: FastLED.addLeds<NEOPIXEL, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 38: FastLED.addLeds<NEOPIXEL, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 39: FastLED.addLeds<NEOPIXEL, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
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
            #if !ARDUINO_USB_CDC_ON_BOOT
                // 19 + 20 = USB HWCDC. reserved for USB port when ARDUINO_USB_CDC_ON_BOOT=1
                case 19: FastLED.addLeds<NEOPIXEL, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 20: FastLED.addLeds<NEOPIXEL, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
                case 21: FastLED.addLeds<NEOPIXEL, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // 22 to 32: not connected, or reserved for SPI FLASH
                // case 22: FastLED.addLeds<NEOPIXEL, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 23: FastLED.addLeds<NEOPIXEL, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 24: FastLED.addLeds<NEOPIXEL, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 25: FastLED.addLeds<NEOPIXEL, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #if !defined(BOARD_HAS_PSRAM)
                // 26-32 = reserved for PSRAM
                case 26: FastLED.addLeds<NEOPIXEL, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 27: FastLED.addLeds<NEOPIXEL, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 28: FastLED.addLeds<NEOPIXEL, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 29: FastLED.addLeds<NEOPIXEL, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 30: FastLED.addLeds<NEOPIXEL, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 31: FastLED.addLeds<NEOPIXEL, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 32: FastLED.addLeds<NEOPIXEL, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
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
                // 46 input-only
                // case 46: FastLED.addLeds<NEOPIXEL, 46>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif //CONFIG_IDF_TARGET_ESP32S2

              #if CONFIG_IDF_TARGET_ESP32C3
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
                // 11-17 reserved for SPI FLASH
                //case 11: FastLED.addLeds<NEOPIXEL, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 12: FastLED.addLeds<NEOPIXEL, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 13: FastLED.addLeds<NEOPIXEL, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 14: FastLED.addLeds<NEOPIXEL, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 15: FastLED.addLeds<NEOPIXEL, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 16: FastLED.addLeds<NEOPIXEL, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 17: FastLED.addLeds<NEOPIXEL, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #if !ARDUINO_USB_CDC_ON_BOOT
                // 18 + 19 = USB HWCDC. reserved for USB port when ARDUINO_USB_CDC_ON_BOOT=1
                case 18: FastLED.addLeds<NEOPIXEL, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 19: FastLED.addLeds<NEOPIXEL, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
                // 20+21 = Serial RX+TX --> don't use for LEDS when serial-to-USB is needed
                case 20: FastLED.addLeds<NEOPIXEL, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 21: FastLED.addLeds<NEOPIXEL, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif //CONFIG_IDF_TARGET_ESP32S2

              #if CONFIG_IDF_TARGET_ESP32S3
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
            #if !ARDUINO_USB_CDC_ON_BOOT
                // 19 + 20 = USB-JTAG. Not recommended for other uses.
                case 19: FastLED.addLeds<NEOPIXEL, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 20: FastLED.addLeds<NEOPIXEL, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
                case 21: FastLED.addLeds<NEOPIXEL, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // // 22 to 32: not connected, or SPI FLASH
                // case 22: FastLED.addLeds<NEOPIXEL, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 23: FastLED.addLeds<NEOPIXEL, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 24: FastLED.addLeds<NEOPIXEL, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 25: FastLED.addLeds<NEOPIXEL, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 26: FastLED.addLeds<NEOPIXEL, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 27: FastLED.addLeds<NEOPIXEL, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 28: FastLED.addLeds<NEOPIXEL, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 29: FastLED.addLeds<NEOPIXEL, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 30: FastLED.addLeds<NEOPIXEL, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 31: FastLED.addLeds<NEOPIXEL, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 32: FastLED.addLeds<NEOPIXEL, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #if !defined(BOARD_HAS_PSRAM)
                // 33 to 37: reserved if using _octal_ SPI Flash or _octal_ PSRAM
                case 33: FastLED.addLeds<NEOPIXEL, 33>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 34: FastLED.addLeds<NEOPIXEL, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 35: FastLED.addLeds<NEOPIXEL, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 36: FastLED.addLeds<NEOPIXEL, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 37: FastLED.addLeds<NEOPIXEL, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
                case 38: FastLED.addLeds<NEOPIXEL, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 39: FastLED.addLeds<NEOPIXEL, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 40: FastLED.addLeds<NEOPIXEL, 40>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 41: FastLED.addLeds<NEOPIXEL, 41>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 42: FastLED.addLeds<NEOPIXEL, 42>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // 43+44 = Serial RX+TX --> don't use for LEDS when serial-to-USB is needed
                case 43: FastLED.addLeds<NEOPIXEL, 43>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 44: FastLED.addLeds<NEOPIXEL, 44>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 45: FastLED.addLeds<NEOPIXEL, 45>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 46: FastLED.addLeds<NEOPIXEL, 46>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 47: FastLED.addLeds<NEOPIXEL, 47>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 48: FastLED.addLeds<NEOPIXEL, 48>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif //CONFIG_IDF_TARGET_ESP32S3

              default: USER_PRINTF("FastLedPin assignment: pin not supported %d\n", pinNr);
            }
          }
        }
        pinNr++;
      }
    }
  } //loop

  void loop1s() {
    mdl->setUIValueV("realFps", "%lu /s", frameCounter);
    frameCounter = 0;
  }

private:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;

};

static AppModLeds *lds;