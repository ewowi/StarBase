#include "AppModLeds.h"

#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModModel.h"
#include "SysModFiles.h"
#include "SysModWeb.h"

//load ledfix json file, parse it and depending on the projection, create a mapping for it
void LedsV::ledFixProjectAndMap(JsonObject ledFixObject, JsonObject projectionObject) {
  char fileName[30] = "";

  if (files->seqNrToName(fileName, ledFixObject["value"])) {
    JsonRDWS jrdws(fileName); //open fileName for deserialize

    ledsV.mappingTableLedCounter = 0;
    ledsV.mappingTable.clear();

    //what to deserialize
    jrdws.lookFor("width", &width);
    jrdws.lookFor("height", &height);
    jrdws.lookFor("depth", &depth);
    jrdws.lookFor("nrOfLeds", &nrOfLedsP);

    uint8_t projection = mdl->getValue("projection");
    if (true || projectionObject["value"] != 0) { //not None
      jrdws.lookFor("leds", [](std::vector<uint16_t> uint16CollectList) {
        // print->print("funList ");
        // for (uint16_t num:uint16CollectList)
        //   print->print(" %d", num);
        // print->print("\n");

        if (uint16CollectList.size()>=1 && uint16CollectList.size()<=3) { //we only comprehend 1D, 2D, 3D 
          uint16_t dist;

          if (uint16CollectList.size() == 1)
            dist = uint16CollectList[0];
          else if (uint16CollectList.size() == 2)
            dist = distance(uint16CollectList[0],uint16CollectList[1],0,0,0,0);
          else if (uint16CollectList.size() == 3)
            dist = distance(uint16CollectList[0],uint16CollectList[1],uint16CollectList[2],0,0,0);

          //add physical tables if not present
          if (dist >= ledsV.mappingTable.size()) {
            for (int i = ledsV.mappingTable.size(); i<=dist;i++) {
              // print->print("mapping add physMap %d %d\n", dist, ledsV.mappingTable.size());
              std::vector<uint16_t> physMap;
              ledsV.mappingTable.push_back(physMap);
            }
          }

          ledsV.mappingTable[dist].push_back(ledsV.mappingTableLedCounter++);

          // print->print("mapping %d V:%d P:%d\n", dist, ledsV.mappingTable.size(), ledsV.mappingTableLedCounter);

          // delay(1); //feed the watchdog
        }
      }); //create the right type, otherwise crash

    } //projection != 0
    if (jrdws.deserialize()) { //find all the objects

      if (true || projectionObject["value"] != 0) { //not None
        nrOfLedsV = ledsV.mappingTable.size();

        uint16_t x=0;
        uint16_t y=0;
        for (std::vector<uint16_t>physMap:ledsV.mappingTable) {
          print->print("led %d mapping: ", x);
          for (uint16_t pos:physMap) {
            print->print(" %d", pos);
            y++;
          }
          print->print("\n");
          x++;
        }
      }
      else
        nrOfLedsV = nrOfLedsP;

      print->print("jrdws whd %d %d %d and P:%d V:%d\n", width, height, depth, nrOfLedsP, nrOfLedsV);

      //at page refresh, done before these objects have been initialized...
      mdl->setValueI("width", width);
      // print->print("1.1\n");
      mdl->setValueI("height", height);
      // print->print("1.2\n");
      mdl->setValueI("depth", depth);
      // print->print("1.3\n");
      mdl->setValueV("nrOfLeds", "P:%d V:%d", nrOfLedsP, nrOfLedsV);

      // print->print("1\n");

      //send to pview a message to get file filename
      JsonDocument *responseDoc = web->getResponseDoc();
      responseDoc->clear(); //needed for deserializeJson?
      JsonVariant responseVariant = responseDoc->as<JsonVariant>();
      // print->print("2\n");

      web->addResponse("pview", "file", fileName);
      web->sendDataWs(responseVariant);
      print->printJson("ledfix chFun send ws done", responseVariant); //during server startup this is not send to a client, so client refresh should also trigger this
      // print->print("3\n");
    } // if deserialize
  } //if fileName
}

// ledsV[indexV] stores indexV locally
LedsV& LedsV::operator[](uint16_t indexV) {
  indexVLocal = indexV;
  return *this;
}

// CRGB& operator[](uint16_t indexV) {
//   // indexVLocal = indexV;
//   CRGB x = getPixelColor(indexV);
//   return x;
// }

// ledsV = uses locally stored indexV and color to call setPixelColor
LedsV& LedsV::operator=(const CRGB color) {
  setPixelColor(indexVLocal, color);
  return *this;
}

// maps the virtual led to the physical led(s) and assign a color to it
void LedsV::setPixelColor(int indexV, CRGB color) {
  if (indexV > mappingTable.size()) return;
  for (uint16_t indexP:mappingTable[indexV]) {
    if (indexP < NUM_LEDS_Preview)
      ledsP[indexP] = color;
  }
}

CRGB LedsV::getPixelColor(int indexV) {
  if (!mappingTable.size() ||indexV > mappingTable.size()) return CRGB::Black;
  if (!mappingTable[indexV].size() || mappingTable[indexV][0] > NUM_LEDS_Preview) return CRGB::Black;

  return ledsP[mappingTable[indexV][0]]; //any would do as they are all the same
}

// LedsV& operator+=(const CRGB color) {
//   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
//   return *this;
// }
// LedsV& operator|=(const CRGB color) {
//   // setPixelColor(indexVLocal, color);
//   setPixelColor(indexVLocal, getPixelColor(indexVLocal) | color);
//   return *this;
// }

// LedsV& operator+(const CRGB color) {
//   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
//   return *this;
// }

AppModLeds::AppModLeds() :Module("Leds") {};

void AppModLeds::setup() {
  Module::setup();
  print->print("%s %s\n", __PRETTY_FUNCTION__, name);

  parentObject = ui->initModule(parentObject, name);

  ui->initSlider(parentObject, "bri", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "Brightness");
  }, [](JsonObject object) { //chFun
    bri = map(object["value"], 0, 100, 0, 255);
    FastLED.setBrightness(bri);
    print->print("Set Brightness to %d -> %d\n", object["value"].as<int>(), bri);
  });

  ui->initSelect(parentObject, "fx", 6, false, [](JsonObject object) { //uiFun. 6: Juggles is default
    web->addResponse(object["id"], "label", "Effect");
    web->addResponse(object["id"], "comment", "Effect to show");
    JsonArray select = web->addResponseA(object["id"], "select");
    for (Effect *effect:effects) {
      select.add(effect->name());
    }
  }, [](JsonObject object) { //chFun
    print->print("%s Change %s to %d\n", "initSelect chFun", object["id"].as<const char *>(), object["value"].as<int>());
  });

  ui->initSelect(parentObject, "projection", 1, false, [](JsonObject object) { //uiFun. 1:  is default
    // web->addResponse(object["id"], "label", "Effect");
    web->addResponse(object["id"], "comment", "How to project fx to fixture");
    JsonArray select = web->addResponseA(object["id"], "select");
    select.add("None");
    select.add("Distance from point");
    select.add("Random");
  }, [](JsonObject object) { //chFun

    print->print("%s Change %s to %d\n", "initSelect chFun", object["id"].as<const char *>(), object["value"].as<int>());

    //
    uint8_t projection = object["value"];
    switch (projection) {
    case 1:
      // calculate mapping table
      break;
    }

    ledsV.ledFixProjectAndMap(mdl->findObject("ledFix"), object);

  });

  ui->initCanvas(parentObject, "pview", map(5, 0, 255, 0, 100), [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "Preview");
    // web->addResponse(object["id"], "comment", "Click to enlarge");
  }, nullptr, [](JsonObject object, uint8_t* buffer) { //loopFun
    // send leds preview to clients
    for (size_t i = 0; i < buffer[0] * 256 + buffer[1]; i++)
    {
      buffer[i*3+4] = ledsP[i].red;
      buffer[i*3+4+1] = ledsP[i].green;
      buffer[i*3+4+2] = ledsP[i].blue;
    }
    //new values
    buffer[0] = nrOfLedsP/256;
    buffer[1] = nrOfLedsP%256;
    buffer[3] = max(nrOfLedsP * web->ws->count()/200, 16U); //interval in ms * 10, not too fast
  });

  ui->initSelect(parentObject, "ledFix", 0, false, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "LedFix");
    JsonArray select = web->addResponseA(object["id"], "select");
    files->dirToJson(select, true, "D"); //only files containing D (1D,2D,3D), alphabetically, only looking for D not very destinctive though

    // ui needs to load the file also initially
    char fileName[30] = "";
    if (files->seqNrToName(fileName, object["value"])) {
      web->addResponse("pview", "file", fileName);
    }
  }, [](JsonObject object) { //chFun
    print->print("%s Change %s to %d\n", "initSelect chFun", object["id"].as<const char *>(), object["value"].as<int>());

    ledsV.ledFixProjectAndMap(object, mdl->findObject("projection"));


  }); //ledFix

  ui->initText(parentObject, "width", nullptr, true, [](JsonObject object) { //uiFun
    web->addResponseV(object["id"], "comment", "Max %dK", 32);
  });

  ui->initText(parentObject, "height", nullptr, true, [](JsonObject object) { //uiFun
    web->addResponseV(object["id"], "comment", "Max %dK", 32);
  });

  ui->initText(parentObject, "depth", nullptr, true, [](JsonObject object) { //uiFun
    web->addResponseV(object["id"], "comment", "Max %dK", 32);
  });

  ui->initText(parentObject, "nrOfLeds", nullptr, true, [](JsonObject object) { //uiFun
    web->addResponseV(object["id"], "comment", "Max %d (%d by FastLed)", NUM_LEDS_Preview, NUM_LEDS_FastLed);
  });

  //set the values by chFun
  print->print("post whd %d %d %d and P:%d V:%d\n", width, height, depth, nrOfLedsP, nrOfLedsV);
  mdl->setValueI("width", width);
  mdl->setValueI("height", height);
  mdl->setValueI("depth", depth);
  mdl->setValueV("nrOfLeds", "P:%d V:%d", nrOfLedsP, nrOfLedsV);

  // if (!leds)
  //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
  // else
  //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
  // if (leds) free(leds);
  // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
  // leds = (CRGB*)reallocarray

  ui->initNumber(parentObject, "fps", fps, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "comment", "Frames per second");
  }, [](JsonObject object) { //chFun
    fps = object["value"];
    print->print("fps changed %d\n", fps);
  });

  ui->initText(parentObject, "realFps", nullptr, true, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "comment", "Depends on how much leds fastled has configured");
  });

  ui->initNumber(parentObject, "dataPin", dataPin, [](JsonObject object) { //uiFun
    web->addResponseV(object["id"], "comment", "Not implemented yet (fixed to %d)", DATA_PIN);
  }, [](JsonObject object) { //chFun
    print->print("Set data pin to %d\n", object["value"].as<int>());
  });

  enum Fixtures
  {
    f_1DSpiral,
    f_2DM88,
    f_2DR24,
    f_2DR35,
    f_3DC885,
    f_3DC888,
    f_3DGlobe,
    f_3DHumanSizedCube
  };

  ui->initSelect(parentObject, "ledFixGen", 3, false, [](JsonObject object) { //uiFun
    web->addResponse(object["id"], "label", "Ledfix generator");
    JsonArray select = web->addResponseA(object["id"], "select");
    select.add("1DSpiral"); //0
    select.add("2DMatrix88"); //1
    select.add("2DRing24"); //2
    select.add("2DRing35"); //3
    select.add("3DCube885"); //4
    select.add("3DCube888"); //5
    select.add("3DGlobe"); //6
    select.add("3DHumanSizedCube"); //7
  }, [](JsonObject object) { //chFun

    const char * name = "TT";
    uint16_t nrOfLeds = 64; //default
    uint16_t diameter = 100; //in mm

    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t depth = 0;

    uint8_t fix = object["value"];
    switch (fix) {
      case f_1DSpiral:
        name = "1DSpiral";
        nrOfLeds = 64;
        break;
      case f_2DM88:
        name = "2DMatrix88";
        nrOfLeds = 64;
        break;
      case f_2DR24:
        name = "2DRing24";
        nrOfLeds = 24;
        break;
      case f_2DR35:
        name = "2DRing35";
        nrOfLeds = 35;
        break;
      case f_3DC885:
        name = "3DCube885";
        nrOfLeds = 320;
        break;
      case f_3DC888:
        name = "3DCube888";
        nrOfLeds = 512;
        break;
      case f_3DGlobe:
        name = "3DGlobe";
        nrOfLeds = 512;
        break;
      case f_3DHumanSizedCube:
        name = "3DHumanSizedCube";
        nrOfLeds = 2000;
        break;
    }

    char fileName[30] = "/";
    strcat(fileName, name);
    strcat(fileName, ".json");

    File f = files->open(fileName, "w");
    if (f) {
      f.print("{"); //{\"pview\":{\"json\":
    }
    else
      print->print("ledFixGen Could not open file %s for writing\n", fileName);
    
    char sep[3]="";
    char sep2[3]="";

    uint8_t pin = 10;

    f.printf("\"name\":\"%s\"", name);
    f.printf(",\"nrOfLeds\":%d", nrOfLeds);
    // f.printf(",\"pin\":%d",16);
    switch (fix) {
      case f_1DSpiral:
      case f_2DR24:
      case f_2DR35:
        diameter = 100; //in mm

        f.printf(",\"scale\":\"%s\"", "mm");
        f.printf(",\"size\":%d", diameter);

        width = 10;
        height = 10;
        depth = 1;
        f.printf(",\"width\":%d", width);
        f.printf(",\"height\":%d", height);
        f.printf(",\"depth\":%d", depth);

        f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
        strcpy(sep, "");
        for (int i=0; i<nrOfLeds; i++) {
          float radians = i*360/nrOfLeds * (M_PI / 180);
          uint16_t x = diameter/2 * (1 + sinf(radians));
          uint8_t y = diameter/2 * (1+ cosf(radians));
          f.printf("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
        }
        f.printf("]}]");
        break;
      case f_2DM88:
        diameter = 8; //in cm

        f.printf(",\"scale\":\"%s\"", "cm");
        f.printf(",\"size\":%d", diameter);

        width = 8;
        height = 8;
        depth = 1;
        f.printf(",\"width\":%d", width);
        f.printf(",\"height\":%d", height);
        f.printf(",\"depth\":%d", depth);

        f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
        strcpy(sep,"");
        for (uint8_t y = 0; y<8; y++)
          for (uint16_t x = 0; x<8 ; x++) {
            // width = max(width, x);
            // height = max(height, y);
            // depth = 1;
            f.printf("%s[%d,%d]", sep, x,y); strcpy(sep, ",");
          }
        f.printf("]}]");

        break;
      case f_3DC888:
        diameter = 8; //in cm

        f.printf(",\"scale\":\"%s\"", "cm");
        f.printf(",\"size\":%d", diameter);

        width = 8;
        height = 8;
        depth = 8;
        f.printf(",\"width\":%d", width);
        f.printf(",\"height\":%d", height);
        f.printf(",\"depth\":%d", depth);

        f.printf(",\"outputs\":[{\"pin\":10,\"leds\":[");
        strcpy(sep,"");
        for (uint8_t z = 0; z<depth; z++)
          for (uint8_t y = 0; y<height; y++)
            for (uint16_t x = 0; x<width ; x++) {
              f.printf("%s[%d,%d,%d]", sep, x,y,z); strcpy(sep, ",");
            }
        f.printf("]}]");

        break;
      case f_3DC885:
      case f_3DHumanSizedCube:
        if (fix==f_3DC885) {
          diameter = 8; //in cm
          width = 8;
          height = 8;
          depth = 8;
          f.printf(",\"scale\":\"%s\"", "cm");
        }
        else {
          diameter = 20; //in dm
          width = 20;
          height = 20;
          depth = 20;
          f.printf(",\"scale\":\"%s\"", "dm");
        }

        f.printf(",\"size\":%d", diameter);

        f.printf(",\"width\":%d", width);
        f.printf(",\"height\":%d", height);
        f.printf(",\"depth\":%d", depth);

        f.printf(",\"outputs\":[");
        strcpy(sep,"");

        //front and back
        for (uint8_t z = 0; z<depth; z+=depth-1) {
          f.printf("%s{\"pin\":%d,\"leds\":[", sep, pin++);strcpy(sep, ",");
          strcpy(sep2,"");
          for (uint8_t y = 0; y<height; y++)
            for (uint16_t x = 0; x<width ; x++) {
              f.printf("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
            }
          f.printf("]}");
        }
        //NO botom and top
        for (uint8_t y = height-1; y<height; y+=height-1) {
          f.printf("%s{\"pin\":%d,\"leds\":[", sep, pin++);strcpy(sep, ",");
          strcpy(sep2,"");
          for (uint8_t z = 0; z<depth; z++)
            for (uint16_t x = 0; x<width ; x++) {
              f.printf("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
            }
          f.printf("]}");
        }

        //left and right
        for (uint16_t x = 0; x<width ; x+=width-1) {
          f.printf("%s{\"pin\":%d,\"leds\":[", sep, pin++);strcpy(sep, ",");
          strcpy(sep2,"");
          for (uint8_t z = 0; z<depth; z++)
            for (uint8_t y = 0; y<height; y++) {
              f.printf("%s[%d,%d,%d]", sep2, x,y,z); strcpy(sep2, ",");
            }
          f.printf("]}");
        }
      
        f.printf("]");

        break;
      case f_3DGlobe:
        diameter = 100; //in mm

        f.printf(",\"scale\":\"%s\"", "mm");
        f.printf(",\"size\":%d", diameter);

        width = 10;
        height = 10;
        depth = 10;
        f.printf(",\"width\":%d", width);
        f.printf(",\"height\":%d", height);
        f.printf(",\"depth\":%d", depth);

        f.printf(",\"leds\":[");
        strcpy(sep, "");
        for (int i=0; i<nrOfLeds; i++) {
          float radians = i*360/nrOfLeds * (M_PI / 180);
          uint16_t x = diameter/2 * (1 + sinf(radians));
          uint8_t y = diameter/2 * (1+ cosf(radians));
          uint8_t z = diameter/2 * (1+ cosf(radians));
          f.printf("%s[%d,%d,%d]", sep, x,y,z); strcpy(sep, ",");

        }
        f.printf("]");
        break;
    }

    f.print("}"); //}}
    f.close();

    files->filesChange();

    //reload ledfix select
    ui->processUiFun("ledFix");
  }); //ledFixGen

  effects.push_back(new RainbowEffect);
  effects.push_back(new RainbowWithGlitterEffect);
  effects.push_back(new SinelonEffect);
  effects.push_back(new RunningEffect);
  effects.push_back(new ConfettiEffect);
  effects.push_back(new BPMEffect);
  effects.push_back(new JuggleEffect);
  effects.push_back(new Ripples3DEffect);
  effects.push_back(new SphereMove3DEffect);

  // FastLED.addLeds<NEOPIXEL, 6>(leds, 1); 
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(ledsP, NUM_LEDS_FastLed); 

  print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void AppModLeds::loop() {
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
