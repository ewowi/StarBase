/*
   @title     StarMod
   @file      AppModLeds.h
   @date      20230807
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "Module.h"

#include "AppLedsV.h"
#include "AppEffects.h"

#include <vector>
#include "FastLED.h"

#define DATA_PIN 16

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

class AppModLeds:public Module {

public:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;

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
      // if (LedsV::projectionNr == p_DistanceFromPoint) {
      if (fx == 9 && LedsV::fxDimension != 2) { // 9 = "Frizzles2D"
        LedsV::fxDimension = 2;
        doMap = true;
      }
      if (fx != 9 && LedsV::fxDimension != 1) { // 9 = "Frizzles2D"
        LedsV::fxDimension = 1;
        doMap = true;
      }

      // if (false) {
      JsonObject parentVar = mdl->findVar(var["id"]);
      parentVar.remove("n"); //tbd: we should also remove the uiFun and chFun !!

      for (int i=0; i<5; i++) {
        uint8_t nameNr = random8(6);  
        uint8_t typeNr = random8(5);  
        char name[12];
        switch (nameNr) {
          case 0:
            strcpy(name, "lorum");
            break;
          case 1:
            strcpy(name, "ipsum");
            break;
          case 2:
            strcpy(name, "dolor");
            break;
          case 3:
            strcpy(name, "sit");
            break;
          case 4:
            strcpy(name, "amet");
            break;
          case 5:
            strcpy(name, "consectetur");
            break;
        }
        
        switch (typeNr) {
          case 0:
            ui->initText(parentVar, name, name, false);
            break;
          case 1:
            ui->initNumber(parentVar, name, random8(255), false);
            break;
          case 2:
            ui->initSlider(parentVar, name, random8(255), false);
            break;
          case 3:
            ui->initCheckBox(parentVar, name, random8(2), false);
            break;
          case 4:
            ui->initSelect(parentVar, name, random8(2), false, [](JsonObject var) { //uiFun
              JsonArray select = web->addResponseA(var["id"], "select");
              select.add("Oui"); //0
              select.add("Non"); //1
            });
            break;
        }
      }
      print->printJson("parentVar", parentVar);
      web->sendDataWs(parentVar); //always send, also when no children, to remove them from ui
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
      // web->addResponseV(var["id"], "comment", "Max %dK", 32);
    }, [](JsonObject var) { //chFun
    });

    ui->initText(parentVar, "nrOfLeds", nullptr, true, [](JsonObject var) { //uiFun
      web->addResponseV(var["id"], "comment", "Max %d (%d by FastLed)", NUM_LEDS_Preview, NUM_LEDS_FastLed);
    });

    //set the values by chFun
    //tbd: add page reload event (as these values should be given each time a page reloads, and they are not included in model.json as they are readonly...
    // print->print("post whd P:%dx%dx%d and P:%d V:%d\n", LedsV::widthP, LedsV::heightP, LedsV::depthP, LedsV::nrOfLedsP, LedsV::nrOfLedsV);
    print->print("post whd P:%dx%dx%d V:%dx%dx%d and P:%d V:%d\n", LedsV::widthP, LedsV::heightP, LedsV::depthP, LedsV::widthV, LedsV::heightV, LedsV::depthV, LedsV::nrOfLedsP, LedsV::nrOfLedsV);
    mdl->setValueV("dimensions", "P:%dx%dx%d V:%dx%dx%d", LedsV::widthP, LedsV::heightP, LedsV::depthP, LedsV::widthV, LedsV::heightV, LedsV::depthV);
    mdl->setValueV("nrOfLeds", "P:%d V:%d", LedsV::nrOfLedsP, LedsV::nrOfLedsV);

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

    // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(ledsP, NUM_LEDS_FastLed); 

    mdl->addWatch(1, "bri", 256);
    mdl->addWatch(2, "fx", effects.size());
    mdl->addWatch(3, "projection", Projections::count);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    if (millis() - frameMillis >= 1000.0/fps) {
      frameMillis = millis();

      Effect* effect = effects[mdl->getValue("fx")];
      effect->loop();

      // yield();
      FastLED.show();  

      frameCounter++;
      call++;
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