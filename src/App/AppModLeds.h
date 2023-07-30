/*
   @title     StarMod
   @file      AppModLeds.h
   @date      20230729
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
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

  AppModLeds() :Module("Leds") {};

  void setup() {
  Module::setup();
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = ui->initModule(parentVar, name);

  ui->initSlider(parentVar, "bri", map(5, 0, 255, 0, 100), [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Brightness");
  }, [](JsonObject var) { //chFun
    uint8_t bri = map(var["value"], 0, 100, 0, 255);
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
    print->print("%s Change %s to %d\n", "initSelect chFun", var["id"].as<const char *>(), var["value"].as<int>());
  });

  ui->initSelect(parentVar, "projection", 0, false, [](JsonObject var) { //uiFun. 1:  is default
    // web->addResponse(var["id"], "label", "Effect");
    web->addResponse(var["id"], "comment", "How to project fx to fixture");
    JsonArray select = web->addResponseA(var["id"], "select");
    select.add("None");
    select.add("Random");
    select.add("Distance from point");
  }, [](JsonObject var) { //chFun
    print->print("%s Change %s to %d\n", "initSelect chFun", var["id"].as<const char *>(), var["value"].as<int>());

    ledsV.projectionNr = var["value"];
    ledsV.ledFixProjectAndMap();
  });

  ui->initCanvas(parentVar, "pview", map(5, 0, 255, 0, 100), [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Preview");
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
    buffer[3] = max(ledsV.nrOfLedsP * web->ws->count()/200, 16U); //interval in ms * 10, not too fast
  });

  ui->initSelect(parentVar, "ledFix", 0, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "LedFix");
    JsonArray select = web->addResponseA(var["id"], "select");
    files->dirToJson(select, true, "D"); //only files containing D (1D,2D,3D), alphabetically, only looking for D not very destinctive though

    // ui needs to load the file also initially
    char fileName[30] = "";
    if (files->seqNrToName(fileName, var["value"])) {
      web->addResponse("pview", "file", fileName);
    }
  }, [](JsonObject var) { //chFun
    print->print("%s Change %s to %d\n", "initSelect chFun", var["id"].as<const char *>(), var["value"].as<int>());

    ledsV.ledFixNr = var["value"];
    ledsV.ledFixProjectAndMap();


  }); //ledFix

  ui->initText(parentVar, "dimensions", nullptr, true, [](JsonObject var) { //uiFun
    // web->addResponseV(var["id"], "comment", "Max %dK", 32);
  });

  ui->initText(parentVar, "nrOfLeds", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponseV(var["id"], "comment", "Max %d (%d by FastLed)", NUM_LEDS_Preview, NUM_LEDS_FastLed);
  });

  //set the values by chFun
  //to do: add page reload event (as these values should be given each time a page reloads, and they are not included in model.json as they are readonly...
  print->print("post whd %d %d %d and P:%d V:%d\n", ledsV.width, ledsV.height, ledsV.depth, ledsV.nrOfLedsP, ledsV.nrOfLedsV);
  mdl->setValueV("dimensions", "%dx%dx%d", ledsV.width, ledsV.height, ledsV.depth);
  mdl->setValueV("nrOfLeds", "P:%d V:%d", ledsV.nrOfLedsP, ledsV.nrOfLedsV);

  ui->initNumber(parentVar, "fps", fps, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Frames per second");
  }, [](JsonObject var) { //chFun
    fps = var["value"];
    print->print("fps changed %d\n", fps);
  });

  ui->initText(parentVar, "realFps", nullptr, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "comment", "Depends on how much leds fastled has configured");
  });

  ui->initNumber(parentVar, "dataPin", DATA_PIN, [](JsonObject var) { //uiFun
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

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

  void loop() {
    // Module::loop();

    if(millis() - frameMillis >= 1000.0/fps) {
      frameMillis = millis();

      Effect* effect = effects[mdl->getValue("fx")];
      effect->loop();

      // yield();
      FastLED.show();  

      frameCounter++;
      call++;
    }
    if (millis() - secondMillis >= 1000 || !secondMillis) {
      secondMillis = millis();
      mdl->setValueV("realFps", "%lu /s", frameCounter);
      frameCounter = 0;
    }

    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }

};

static AppModLeds *lds;

uint16_t AppModLeds::fps = 40;