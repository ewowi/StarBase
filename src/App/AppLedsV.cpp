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

std::vector<std::vector<uint16_t>> LedsV::mappingTable;
uint16_t LedsV::mappingTableLedCounter = 0;
uint16_t LedsV::nrOfLedsP = 64; //amount of physical leds
uint16_t LedsV::nrOfLedsV = 64;  //amount of virtual leds (calculated by projection)
uint16_t LedsV::widthP = 8; 
uint16_t LedsV::heightP = 8; 
uint16_t LedsV::depthP = 1; 
uint16_t LedsV::widthV = 8; 
uint16_t LedsV::heightV = 8; 
uint16_t LedsV::depthV = 1; 

uint8_t LedsV::projectionNr = -1;
uint8_t LedsV::ledFixNr = -1;
uint8_t LedsV::fxDimension = -1;

//load ledfix json file, parse it and depending on the projection, create a mapping for it
void LedsV::ledFixProjectAndMap() {
  char fileName[32] = "";

  if (files->seqNrToName(fileName, ledFixNr)) {
    JsonRDWS jrdws(fileName); //open fileName for deserialize

    mappingTableLedCounter = 0;

    //vectors really gone now?
    for (std::vector<uint16_t> physMap: mappingTable)
      physMap.clear();
    mappingTable.clear();

    //deallocate all led pins
    uint8_t pinNr = 0;
    for (PinObject pinObject: SysModPins::pinObjects) {
      if (strcmp(pinObject.owner, "Leds") == 0)
        pins->deallocatePin(pinNr, "Leds");
      pinNr++;
    }

    //track pins and leds
    static uint8_t currPin;
    static uint16_t prevLeds;
    prevLeds = 0;

    //what to deserialize
    jrdws.lookFor("width", &widthP);
    jrdws.lookFor("height", &heightP);
    jrdws.lookFor("depth", &depthP);
    jrdws.lookFor("nrOfLeds", &nrOfLedsP);
    jrdws.lookFor("pin", &currPin);

    //lookFor leds array and for each item in array call lambdo to make a projection
    jrdws.lookFor("leds", [](std::vector<uint16_t> uint16CollectList) { //this will be called for each tuple of coordinates!
      // USER_PRINTF("funList ");
      // for (uint16_t num:uint16CollectList)
      //   USER_PRINTF(" %d", num);
      // USER_PRINTF("\n");

      uint8_t ledFixDimension = uint16CollectList.size();

      if (ledFixDimension>=1 && ledFixDimension<=3) { //we only comprehend 1D, 2D, 3D 

        uint16_t x = uint16CollectList[0] / 10;
        uint16_t y = (ledFixDimension>=2)?uint16CollectList[1] / 10 : 1;
        uint16_t z = (ledFixDimension>=3)?uint16CollectList[2] / 10 : 1;

        // USER_PRINTF("projectionNr p:%d f:%d s:%d, %d-%d-%d %d-%d-%d\n", LedsV::projectionNr, LedsV::fxDimension, ledFixDimension, x, y, z, uint16CollectList[0], uint16CollectList[1], uint16CollectList[2]);
        if (LedsV::projectionNr == p_DistanceFromPoint || LedsV::projectionNr == p_DistanceFromCentre) {
          uint16_t bucket;// = -1;
          if (LedsV::fxDimension == 1) { //if effect is 1D

            uint16_t pointX, pointY, pointZ;
            if (LedsV::projectionNr == p_DistanceFromPoint) {
              pointX = 0;
              pointY = 0;
              pointZ = 0;
            } else {
              pointX = LedsV::widthP / 2;
              pointY = LedsV::heightP / 2;
              pointZ = LedsV::depthP / 2;
            }

            if (ledFixDimension == 1) //ledfix is 1D
              bucket = x;
            else if (ledFixDimension == 2) {//ledfix is 2D 
              bucket = distance(x,y,0,pointX,pointY,0);
              // USER_PRINTF("bucket %d-%d %d-%d %d\n", x,y, pointX, pointY, bucket);
            }
            else if (ledFixDimension == 3) //ledfix is 3D
              bucket = distance(x,y,z,pointX, pointY, pointZ);

          }
          else if (LedsV::fxDimension == 2) { //effect is 2D
            depthV = 1;
            if (ledFixDimension == 1) //ledfix is 1D
              bucket = x;
            else if (ledFixDimension == 2) {//ledfix is 2D
              widthV = widthP;
              heightV = heightP;
              depthV = 1;
              float scale = 1;
              if (widthV * heightV > 256)
                scale = sqrt((float)256.0 / (widthV * heightV));
              widthV *= scale;
              heightV *= scale;
              x = (x+1) * scale - 1;
              y = (y+1) * scale - 1;
              bucket = x + y * widthV;
              // USER_PRINTF("2D to 2D bucket %f %d  %d x %d %d x %d\n", scale, bucket, x, y, widthV, heightV);
            }
            else if (ledFixDimension == 3) {//ledfix is 3D
              widthV = widthP + heightP;
              heightV = depthP;
              depthV = 1;
              bucket = (x + y + 1) + z * widthV;
              // USER_PRINTF("2D to 3D bucket %d %d\n", bucket, widthV);
            }
          }
          //tbd: effect is 3D

          if (bucket != -1) {
            //add physical tables if not present
            if (bucket >= NUM_LEDS_Preview) {
              USER_PRINTF("mapping add physMap %d %d too big\n", bucket, mappingTable.size());
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
        mappingTableLedCounter++;
      } //if 1D-3D
      else { // end of leds array

        //check if pin already allocated, if so, extend range in details
        PinObject pinObject = SysModPins::pinObjects[currPin];
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

            strncpy(SysModPins::pinObjects[currPin].details, details, sizeof(PinObject::details)-1);  
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

      USER_PRINTF("ledFixProjectAndMap P:%dx%dx%d V:%dx%dx%d and P:%d V:%d\n", widthP, heightP, depthP, widthV, heightV, depthV, nrOfLedsP, nrOfLedsV);
      mdl->setValueV("dimensions", "P:%dx%dx%d V:%dx%dx%d", LedsV::widthP, LedsV::heightP, LedsV::depthP, LedsV::widthV, LedsV::heightV, LedsV::depthV);
      mdl->setValueV("nrOfLeds", "P:%d V:%d", nrOfLedsP, nrOfLedsV);

    } // if deserialize
  } //if fileName
  else
    USER_PRINTF("ledFixProjectAndMap: Filename for ledfix %d not found\n", ledFixNr);
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

