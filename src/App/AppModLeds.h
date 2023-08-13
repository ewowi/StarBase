/*
   @title     StarMod
   @file      AppModLeds.h
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "Module.h"

#include "AppLedsV.h"
#include "AppEffects.h"
#ifdef USERMOD_E131
  #include "../User/UserModE131.h"
#endif

#include <vector>
#include "FastLED.h"

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

class AppModLeds:public Module {

public:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;
  bool newFrame = false; //for other modules (DDP)

  //need to make these static as they are called in lambda functions 
  static uint16_t fps;
  unsigned long lastMappingMillis = 0;
  static bool doMap;

  AppModLeds() :Module("Leds") {};

  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentVar = ui->initModule(parentVar, name);

    ui->initSlider(parentVar, "bri", 5, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Brightness");
    }, [](JsonObject var) { //chFun
      uint8_t bri = var["value"];
      FastLED.setBrightness(bri);
      print->print("Set Brightness to %d -> %d\n", var["value"].as<int>(), bri);
    });

    ui->initSelect(parentVar, "fx", 0, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Effect");
      web->addResponse(var["id"], "comment", "Effect to show");
      JsonArray select = web->addResponseA(var["id"], "select");
      for (Effect *effect:effects) {
        select.add(effect->name());
      }
    }, [](JsonObject var) { //chFun
      uint8_t fx = var["value"];
      print->print("%s Change %s to %d\n", "initSelect chFun", var["id"].as<const char *>(), fx);

      if (fx < effects.size()) {

        //tbd: make property of effects
        if (strstr(effects[fx]->name(), "2D")) {
          if (LedsV::fxDimension != 2) {
            LedsV::fxDimension = 2;
            doMap = true;
          }
        }
        else if (strstr(effects[fx]->name(), "3D")) {
          if (LedsV::fxDimension != 3) {
            LedsV::fxDimension = 3;
            doMap = true;
          }
        }
        else {
          if (LedsV::fxDimension != 1) {
            LedsV::fxDimension = 1;
            doMap = true;
          }
        }

        JsonObject parentVar = mdl->findVar(var["id"]);
        parentVar.remove("n"); //tbd: we should also remove the uiFun and chFun !!

        Effect* effect = effects[fx];
        effect->setup(); //if changed then run setup once (like call==0 in WLED)
        effect->controls(parentVar);

        print->printJson("parentVar", parentVar);
        web->sendDataWs(parentVar); //always send, also when no children, to remove them from ui
      } // fx < size
    });

    ui->initSelect(parentVar, "projection", 0, false, [](JsonObject var) { //uiFun.
      // web->addResponse(var["id"], "label", "Effect");
      web->addResponse(var["id"], "comment", "How to project fx to fixture");
      JsonArray select = web->addResponseA(var["id"], "select");
      select.add("None"); // 0
      select.add("Random"); // 1
      select.add("Distance from point"); //2
      select.add("Distance from centre"); //3
    }, [](JsonObject var) { //chFun
      print->print("%s Change %s to %d\n", "initSelect chFun", var["id"].as<const char *>(), var["value"].as<int>());

      LedsV::projectionNr = var["value"];
      doMap = true;
    });

    ui->initCanvas(parentVar, "pview", -1, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Preview");
      web->addResponse(var["id"], "comment", "Shows the preview");
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
      buffer[0] = LedsV::nrOfLedsP/256;
      buffer[1] = LedsV::nrOfLedsP%256;
      buffer[3] = max(LedsV::nrOfLedsP * SysModWeb::ws->count()/200, 16U); //interval in ms * 10, not too fast
    });

    ui->initSelect(parentVar, "ledFix", 0, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "LedFix");
      web->addResponse(var["id"], "comment", "Fixture to display effect on");
      JsonArray select = web->addResponseA(var["id"], "select");
      files->dirToJson(select, true, "D"); //only files containing D (1D,2D,3D), alphabetically, only looking for D not very destinctive though

      // ui needs to load the file also initially
      char fileName[30] = "";
      if (files->seqNrToName(fileName, var["value"])) {
        web->addResponse("pview", "file", fileName);
      }
    }, [](JsonObject var) { //chFun
      print->print("%s Change %s to %d\n", "initSelect chFun", var["id"].as<const char *>(), var["value"].as<int>());

      LedsV::ledFixNr = var["value"];
      doMap = true;

      char fileName[30] = "";
      if (files->seqNrToName(fileName, LedsV::ledFixNr)) {
        //send to pview a message to get file filename
        JsonDocument *responseDoc = web->getResponseDoc();
        responseDoc->clear(); //needed for deserializeJson?
        JsonVariant responseVariant = responseDoc->as<JsonVariant>();

        web->addResponse("pview", "file", fileName);
        web->sendDataWs(responseVariant);
        print->printJson("ledfix chFun send ws done", responseVariant); //during server startup this is not send to a client, so client refresh should also trigger this
      }
    }); //ledFix

    ui->initText(parentVar, "dimensions", nullptr, true, [](JsonObject var) { //uiFun
      char details[32] = "";
      print->fFormat(details, sizeof(details), "P:%dx%dx%d V:%dx%dx%d", LedsV::widthP, LedsV::heightP, LedsV::depthP, LedsV::widthV, LedsV::heightV, LedsV::depthV);
      web->addResponse(var["id"], "value", details);
    });

    ui->initText(parentVar, "nrOfLeds", nullptr, true, [](JsonObject var) { //uiFun
      char details[32] = "";
      print->fFormat(details, sizeof(details), "P:%d V:%d", LedsV::nrOfLedsP, LedsV::nrOfLedsV);
      web->addResponse(var["id"], "value", details);
      web->addResponseV(var["id"], "comment", "Max %d", NUM_LEDS_Preview);
    });

    ui->initNumber(parentVar, "fps", fps, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Frames per second");
    }, [](JsonObject var) { //chFun
      AppModLeds::fps = var["value"];
      print->print("fps changed %d\n", AppModLeds::fps);
    });

    ui->initText(parentVar, "realFps", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "comment", "Depends on how much leds fastled has configured");
    });

    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new RunningEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new JuggleEffect);
    effects.push_back(new Ripples3DEffect);
    effects.push_back(new SphereMove3DEffect);
    effects.push_back(new Frizzles2D);
    effects.push_back(new Lines2D);
    effects.push_back(new DistortionWaves2D);
    effects.push_back(new BouncingBalls1D);

    #ifdef USERMOD_E131
      e131mod->addWatch(1, "bri", 256);
      e131mod->addWatch(2, "fx", effects.size());
    #endif

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    //set new frame
    if (millis() - frameMillis >= 1000.0/fps) {
      frameMillis = millis();

      newFrame = true;

      Effect* effect = effects[mdl->getValue("fx")];
      effect->loop();

      FastLED.show();  

      frameCounter++;
    }
    else {
      newFrame = false;
    }

    //update ui
    if (millis() - secondMillis >= 1000) {
      secondMillis = millis();
      mdl->setValueV("realFps", "%lu /s", frameCounter);
      frameCounter = 0;
    }

    //update projection
    if (millis() - lastMappingMillis >= 1000 && doMap) { //not more then once per second (for E131)
      lastMappingMillis = millis();
      doMap = false;
      ledsV.ledFixProjectAndMap();

      uint8_t pinNr=0;
      for (PinObject pinObject:SysModPins::pinObjects) {
        if (strcmp(pinObject.owner, "Leds")== 0) {
          //dirty trick to decode nrOfLedsPerPin
          char * after = strtok((char *)pinObject.details, "-");
          if (after != NULL ) {
            char * before;
            before = after;
            after = strtok(NULL, " ");
            uint16_t nrOfLeds = atoi(after) - atoi(before) + 1;
            uint16_t startLed = atoi(before);
            print->print("FastLED.addLeds new %d: %d-%d\n", pinNr, startLed, nrOfLeds);

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
              default: print->print("FastLedPin assignment: pin not supported %d\n", pinNr);
            }
          }
        }
        pinNr++;
      }
    }
  } //loop

};

static AppModLeds *lds;

uint16_t AppModLeds::fps = 40;
bool AppModLeds::doMap = false;