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

#define DATA_PIN 16

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

class AppModLeds:public Module {

public:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;
  bool newFrame = false;

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

    ui->initSelect(parentVar, "fx", 6, false, [](JsonObject var) { //uiFun. 6: Juggles is default
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

    ui->initSelect(parentVar, "projection", -1, false, [](JsonObject var) { //uiFun. 1:  is default
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

    ui->initSelect(parentVar, "ledFix", -1, false, [](JsonObject var) { //uiFun
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
    }, [](JsonObject var) { //chFun
    });

    ui->initText(parentVar, "nrOfLeds", nullptr, true, [](JsonObject var) { //uiFun
      char details[32] = "";
      print->fFormat(details, sizeof(details), "P:%d V:%d", LedsV::nrOfLedsP, LedsV::nrOfLedsV);
      web->addResponse(var["id"], "value", details);
      web->addResponseV(var["id"], "comment", "Max %d (%d by FastLed)", NUM_LEDS_Preview, NUM_LEDS_FastLed);
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

    ui->initNumber(parentVar, "dataPin", DATA_PIN, false, [](JsonObject var) { //uiFun
      web->addResponseV(var["id"], "comment", "Not implemented yet (fixed to %d)", DATA_PIN);
    }, [](JsonObject var) { //chFun
      print->print("Set data pin to %d\n", var["value"].as<int>());
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

    // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(ledsP, NUM_LEDS_FastLed); 

    #ifdef USERMOD_E131
      e131mod->addWatch(1, "bri", 256);
      e131mod->addWatch(2, "fx", effects.size());
    #endif

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    if (millis() - frameMillis >= 1000.0/fps) {
      frameMillis = millis();

      newFrame = true;

      Effect* effect = effects[mdl->getValue("fx")];
      effect->loop();

      // yield();
      FastLED.show();  

      frameCounter++;
      call++;
    }
    else {
      newFrame = false;
    }
    if (millis() - secondMillis >= 1000) {
      secondMillis = millis();
      mdl->setValueV("realFps", "%lu /s", frameCounter);
      frameCounter = 0;
    }

   if (millis() - lastMappingMillis >= 1000 && doMap) { //not more then once per second (for E131)
      lastMappingMillis = millis();
      doMap = false;
      ledsV.ledFixProjectAndMap();
    }

    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }

};

static AppModLeds *lds;

uint16_t AppModLeds::fps = 40;
bool AppModLeds::doMap = false;