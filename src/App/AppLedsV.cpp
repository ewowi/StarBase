/*
   @title     StarMod
   @file      AppModLeds.cpp
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "AppLedsV.h"

#include "../Sys/SysModPrint.h"
#include "../Sys/SysModModel.h"
#include "../Sys/SysModFiles.h"
#include "../Sys/SysModWeb.h"
#include "../Sys/SysJsonRDWS.h"
#include "../Sys/SysModPins.h"

#define _1D 1
#define _2D 2
#define _3D 3

//load fixture json file, parse it and depending on the projection, create a mapping for it
void LedsV::fixtureProjectAndMap() {
  char fileName[32] = "";

  if (files->seqNrToName(fileName, fixtureNr)) {
    JsonRDWS jrdws(fileName); //open fileName for deserialize

    mappingTableLedCounter = 0;

    //vectors really gone now?
    for (std::vector<uint16_t> physMap: mappingTable)
      physMap.clear();
    mappingTable.clear();

    //deallocate all led pins
    uint8_t pinNr = 0;
    for (PinObject pinObject: pins->pinObjects) {
      if (strcmp(pinObject.owner, "Leds") == 0)
        pins->deallocatePin(pinNr, "Leds");
      pinNr++;
    }

    prevLeds = 0;

    //pre-processing
    switch(projectionNr) {
      case p_None:
        break;
      case p_Random:
        break;
      case p_DistanceFromPoint:
        pointX = 0;
        pointY = 0;
        pointZ = 0;
        break;
      case p_DistanceFromCentre:
        pointX = widthP / 2;
        pointY = heightP / 2;
        pointZ = depthP / 2;
        break;
    }

    //what to deserialize
    jrdws.lookFor("width", &widthP);
    jrdws.lookFor("height", &heightP);
    jrdws.lookFor("depth", &depthP);
    jrdws.lookFor("nrOfLeds", &nrOfLedsP);
    jrdws.lookFor("pin", &currPin);

    //lookFor leds array and for each item in array call lambdo to make a projection
    jrdws.lookFor("leds", [this](std::vector<uint16_t> uint16CollectList) { //this will be called for each tuple of coordinates!
      // USER_PRINTF("funList ");
      // for (uint16_t num:uint16CollectList)
      //   USER_PRINTF(" %d", num);
      // USER_PRINTF("\n");

      uint8_t fixtureDimension = 0;
      if (widthP>1) fixtureDimension++;
      if (heightP>1) fixtureDimension++;
      if (depthP>1) fixtureDimension++;

      if (uint16CollectList.size()>=1 && fixtureDimension>=1 && fixtureDimension<=3) {

        uint16_t x = uint16CollectList[0] / 10;
        uint16_t y = (fixtureDimension>=2)?uint16CollectList[1] / 10 : 1;
        uint16_t z = (fixtureDimension>=3)?uint16CollectList[2] / 10 : 1;

        // USER_PRINTF("projectionNr p:%d f:%d s:%d, %d-%d-%d %d-%d-%d\n", projectionNr, effectDimension, fixtureDimension, x, y, z, uint16CollectList[0], uint16CollectList[1], uint16CollectList[2]);

        //processing: buckets
        uint16_t bucket = -1;
        switch(projectionNr) {
          case p_None:
            break;
          case p_Random:
            break;
          case p_DistanceFromPoint:
          case p_DistanceFromCentre:
            if (effectDimension == _1D) {
              if (fixtureDimension == _1D)
                bucket = distance(x,0, 0,pointX,0,0);
              else if (fixtureDimension == _2D) { 
                bucket = distance(x,y,0,pointX,pointY,0);
                // USER_PRINTF("bucket %d-%d %d-%d %d\n", x,y, pointX, pointY, bucket);
              }
              else if (fixtureDimension == _3D)
                bucket = distance(x,y,z,pointX, pointY, pointZ);
            }
            else if (effectDimension == _2D) {
              depthV = 1; //no 3D
              if (fixtureDimension == _1D)
                bucket = x;
              else if (fixtureDimension == _2D) {
                widthV = widthP;
                heightV = heightP;
                depthV = 1;

                //scaling (check rounding errors)
                //1024 crash in makebuffer...
                float scale = 1;
                if (widthV * heightV > 256)
                  scale = (sqrt((float)256.0 / (widthV * heightV))); //avoid very high virtual resolutions
                widthV *= scale;
                heightV *= scale;
                x = (x+1) * scale - 1;
                y = (y+1) * scale - 1;

                bucket = XY(x, y);
                // USER_PRINTF("2D to 2D bucket %f %d  %d x %d %d x %d\n", scale, bucket, x, y, widthV, heightV);
              }
              else if (fixtureDimension == _3D) {
                widthV = widthP + heightP;
                heightV = depthP;
                depthV = 1;
                bucket = XY(x + y + 1, z);
                // USER_PRINTF("2D to 3D bucket %d %d\n", bucket, widthV);
              }
            }
            //tbd: effect is 3D
            break;
          case p_Reverse:
            break;
          case p_Mirror:
            break;
          case p_Multiply:
            break;
          case p_Fun: //first attempt for distance from Circle 2D
            if (effectDimension == _2D) {
              depthV = 1; //no 3D
              if (fixtureDimension == _2D) {

                float xNew = sin(x * TWO_PI / (float)(widthV-1)) * widthV;
                float yNew = cos(x * TWO_PI / (float)(widthV-1)) * heightV;

                xNew = round(((heightV-1.0-y)/(heightV-1.0) * xNew + widthV) / 2.0);
                yNew = round(((heightV-1.0-y)/(heightV-1.0) * yNew + heightV) / 2.0);

                USER_PRINTF(" %d,%d->%f,%f->%f,%f", x, y, sin(x * TWO_PI / (float)(widthP-1)), cos(x * TWO_PI / (float)(widthP-1)), xNew, yNew);
                x = xNew;
                y = yNew;

                widthV = widthP;
                heightV = heightP;
                depthV = 1;

                bucket = XY(x, y);
                // USER_PRINTF("2D to 2D bucket %f %d  %d x %d %d x %d\n", scale, bucket, x, y, widthV, heightV);
              }
            }
            break;
        }

        if (bucket != (uint16_t)-1) {
          //post processing: inverse mapping 
          switch(projectionNr) {
          case p_DistanceFromCentre:
            switch (effectDimension) {
            case _2D: 
              switch (fixtureDimension) {
              case _2D: 
                float minDistance = 10;
                // USER_PRINTF("checking bucket %d\n", bucket);
                for (uint16_t y=0; y<heightV && minDistance > 0.5; y++)
                for (uint16_t x=0; x<widthV && minDistance > 0.5; x++) {

                  float xNew = sin(x * TWO_PI / (float)(widthV-1)) * widthV;
                  float yNew = cos(x * TWO_PI / (float)(widthV-1)) * heightV;

                  xNew = round(((heightV-1.0-y)/(heightV-1.0) * xNew + widthV) / 2.0);
                  yNew = round(((heightV-1.0-y)/(heightV-1.0) * yNew + heightV) / 2.0);

                  // USER_PRINTF(" %d,%d->%f,%f->%f,%f", x, y, sin(x * TWO_PI / (float)(widthP-1)), cos(x * TWO_PI / (float)(widthP-1)), xNew, yNew);

                  float distance = abs(bucket - xNew - yNew * widthV);

                  //this should work (better) but needs more testing
                  // if (distance < minDistance) {
                  //   minDistance = distance;
                  //   bucket = x+y*widthV;
                  // }

                  if (bucket == (uint8_t)xNew + (uint8_t)yNew * widthV) {
                    // USER_PRINTF("  found one %d => %d=%d+%d*%d (%f+%f*%d) [%f]\n", bucket, x+y*widthV, x,y, widthV, xNew, yNew, widthV, distance);
                    bucket = XY(x, y);
                    minDistance = 0; // stop looking further
                  }
                }
                if (minDistance > 0.5) bucket = -1;
                break;
              }
              break;
            }
            break;
          }

          if (bucket != (uint16_t)-1) {
            //add physical tables if not present
            if (bucket >= NUM_LEDS_Preview) {
              USER_PRINTF("mapping add physMap %d>=%d (%d) too big %d\n", bucket, NUM_LEDS_Preview, mappingTable.size(), (uint16_t)-1);
            }
            else {
              if (bucket >= mappingTable.size()) {
                for (int i = mappingTable.size(); i<=bucket;i++) {
                  // USER_PRINTF("mapping add physMap %d %d\n", bucket, mappingTable.size());
                  std::vector<uint16_t> physMap;
                  mappingTable.push_back(physMap);
                }
              }
              mappingTable[bucket].push_back(mappingTableLedCounter);
            }
          }
        }

        // USER_PRINTF("mapping %d V:%d P:%d\n", dist, mappingTable.size(), mappingTableLedCounter);

        // delay(1); //feed the watchdog
        mappingTableLedCounter++; //also increase if no buffer created
      } //if 1D-3D
      else { // end of leds array

        //check if pin already allocated, if so, extend range in details
        PinObject pinObject = pins->pinObjects[currPin];
        char details[32] = "";
        if (strcmp(pinObject.owner, "Leds") == 0) { //if owner

          char * after = strtok((char *)pinObject.details, "-");
          if (after != NULL ) {
            char * before;
            before = after;
            after = strtok(NULL, " ");
            uint16_t startLed = atoi(before);
            uint16_t nrOfLeds = atoi(after) - atoi(before) + 1;
            print->fFormat(details, sizeof(details)-1, "%d-%d", min(prevLeds, startLed), max((uint16_t)(mappingTableLedCounter - 1), nrOfLeds)); //careful: AppModLeds:loop uses this to assign to FastLed
            USER_PRINTF("pins extend leds %d: %s\n", currPin, details);
            //tbd: more check

            strncpy(pins->pinObjects[currPin].details, details, sizeof(PinObject::details)-1);  
          }
        }
        else {//allocate new pin
          //tbd: check if free
          print->fFormat(details, sizeof(details)-1, "%d-%d", prevLeds, mappingTableLedCounter - 1); //careful: AppModLeds:loop uses this to assign to FastLed
          USER_PRINTF("pins %d: %s\n", currPin, details);
          pins->allocatePin(currPin, "Leds", details);
        }

        prevLeds = mappingTableLedCounter;
      }
    }); //create the right type, otherwise crash

    if (jrdws.deserialize(false)) {

      if (projectionNr <= p_Random) {
        //defaults
        widthV = widthP;
        heightV = heightP;
        depthV = depthP;
        nrOfLedsV = nrOfLedsP;
      }

      if (projectionNr > p_Random) {
        nrOfLedsV = mappingTable.size();

        // uint16_t x=0;
        // uint16_t y=0;
        // for (std::vector<uint16_t>physMap:mappingTable) {
        //   if (physMap.size()) {
        //     USER_PRINTF("ledV %d mapping: firstLedP: %d #ledsP: %d", x, physMap[0], physMap.size());
        //     // for (uint16_t pos:physMap) {
        //     //   USER_PRINTF(" %d", pos);
        //     //   y++;
        //     // }
        //     USER_PRINTF("\n");
        //   }
        //   x++;
        // }
      }

      USER_PRINTF("fixtureProjectAndMap P:%dx%dx%d V:%dx%dx%d and P:%d V:%d\n", widthP, heightP, depthP, widthV, heightV, depthV, nrOfLedsP, nrOfLedsV);
      mdl->setValueV("dimensions", "P:%dx%dx%d V:%dx%dx%d", widthP, heightP, depthP, widthV, heightV, depthV);
      mdl->setValueV("nrOfLeds", "P:%d V:%d", nrOfLedsP, nrOfLedsV);

    } // if deserialize
  } //if fileName
  else
    USER_PRINTF("fixtureProjectAndMap: Filename for fixture %d not found\n", fixtureNr);
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
  if (mappingTable.size()) {
    if (indexV >= mappingTable.size()) return;
    for (uint16_t indexP:mappingTable[indexV]) {
      if (indexP < NUM_LEDS_Preview)
        ledsPhysical[indexP] = color;
    }
  }
  else //no projection
    ledsPhysical[projectionNr==p_Random?random(nrOfLedsP):indexV] = color;
}

CRGB LedsV::getPixelColor(int indexV) {
  if (mappingTable.size()) {
    if (indexV >= mappingTable.size()) return CRGB::Black;
    if (!mappingTable[indexV].size() || mappingTable[indexV][0] > NUM_LEDS_Preview) return CRGB::Black;

    return ledsPhysical[mappingTable[indexV][0]]; //any would do as they are all the same
  }
  else //no projection
    return ledsPhysical[indexV];
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

