/*
   @title     StarMod
   @file      AppModLeds.cpp
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#include "AppLedsV.h"

#include "../Sys/SysModPrint.h"
#include "../Sys/SysModModel.h"
#include "../Sys/SysModFiles.h"
#include "../Sys/SysModWeb.h"
#include "../Sys/SysJsonRDWS.h"

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
  char fileName[30] = "";

  if (files->seqNrToName(fileName, ledFixNr)) {
    JsonRDWS jrdws(fileName); //open fileName for deserialize

    mappingTableLedCounter = 0;
    mappingTable.clear();

    //what to deserialize
    jrdws.lookFor("width", &widthP);
    jrdws.lookFor("height", &heightP);
    jrdws.lookFor("depth", &depthP);
    jrdws.lookFor("nrOfLeds", &nrOfLedsP);

    //define leds mapping
    if (projectionNr > p_Random) { //0 and 1 (none, random have no mapping)

      //lookFor leds array and for each item in array call lambdo to make a projection
      jrdws.lookFor("leds", [](std::vector<uint16_t> uint16CollectList) { //this will be called for each tuple of coordinates!
        // print->print("funList ");
        // for (uint16_t num:uint16CollectList)
        //   print->print(" %d", num);
        // print->print("\n");

        if (uint16CollectList.size()>=1 && uint16CollectList.size()<=3) { //we only comprehend 1D, 2D, 3D 
          // print->print("projectionNr p:%d f:%d s:%d\n", LedsV::projectionNr, LedsV::fxDimension, uint16CollectList.size());
          if (LedsV::projectionNr == p_DistanceFromPoint) {
            uint16_t bucket;// = -1;
            if (LedsV::fxDimension == 1) { //we do distance from point
              //if effect is 1D

              if (uint16CollectList.size() == 1) //ledfix is 1D
                bucket = uint16CollectList[0];
              else if (uint16CollectList.size() == 2) //ledfix is 2D
                bucket = distance(uint16CollectList[0],uint16CollectList[1],0,0,0,0);
              else if (uint16CollectList.size() == 3) //ledfix is 3D
                bucket = distance(uint16CollectList[0],uint16CollectList[1],uint16CollectList[2],0,0,0);

            }
            else if (LedsV::fxDimension == 2) { //we do distance from x, y+z
              depthV = 1;
              if (uint16CollectList.size() == 1) //ledfix is 1D
                bucket = uint16CollectList[0];
              else if (uint16CollectList.size() == 2) //ledfix is 2D
                bucket = distance(uint16CollectList[0],uint16CollectList[1],0,0,0,0);
              else if (uint16CollectList.size() == 3) {//ledfix is 3D
                widthV = widthP + heightP;
                heightV = depthP;
                depthV = 1;
                bucket = (uint16CollectList[0] + uint16CollectList[1] + 1)  + uint16CollectList[2] * widthV;
                // print->print("2D to 3D bucket %d %d\n", bucket, widthV);
              }
            }

            if (bucket != -1) {
              //add physical tables if not present
              if (bucket >= mappingTable.size()) {
                for (int i = mappingTable.size(); i<=bucket;i++) {
                  // print->print("mapping add physMap %d %d\n", bucket, mappingTable.size());
                  std::vector<uint16_t> physMap;
                  mappingTable.push_back(physMap);
                }
              }

              mappingTable[bucket].push_back(mappingTableLedCounter++);
            }
          }

          // print->print("mapping %d V:%d P:%d\n", dist, mappingTable.size(), mappingTableLedCounter);

          // delay(1); //feed the watchdog
        }
      }); //create the right type, otherwise crash

    } //projection != 0

    if (jrdws.deserialize(projectionNr <= p_Random)) { //find all the vars, for p_None and p_random lazy is enough, no need to go through the whole file

      if (projectionNr <= p_Random) {
        //defaults
        widthV = widthP;
        heightV = heightP;
        depthV = depthP;
        nrOfLedsV = nrOfLedsP;
      }

      if (projectionNr > p_Random) {
        nrOfLedsV = mappingTable.size();

        uint16_t x=0;
        uint16_t y=0;
        for (std::vector<uint16_t>physMap:mappingTable) {
          if (physMap.size()) {
            print->print("ledV %d mapping: firstLedP: %d #ledsP: %d", x, physMap[0], physMap.size());
            // for (uint16_t pos:physMap) {
            //   print->print(" %d", pos);
            //   y++;
            // }
            print->print("\n");
          }
          x++;
        }
      }

      print->print("jrdws whd P:%dx%dx%d V:%dx%dx%d and P:%d V:%d\n", widthP, heightP, depthP, widthV, heightV, depthV, nrOfLedsP, nrOfLedsV);

      //at page refresh, done before these vars have been initialized...
      mdl->setValueV("dimensions", "P:%dx%dx%d V:%dx%dx%d", LedsV::widthP, LedsV::heightP, LedsV::depthP, LedsV::widthV, LedsV::heightV, LedsV::depthV);
      mdl->setValueV("nrOfLeds", "P:%d V:%d", nrOfLedsP, nrOfLedsV);
    } // if deserialize
  } //if fileName
  else
    print->print("ledFixProjectAndMap: Filename for ledfix %d not found\n", ledFixNr);
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
        ledsP[indexP] = color;
    }
  }
  else //no projection
    ledsP[projectionNr==p_Random?random(nrOfLedsP):indexV] = color;
}

CRGB LedsV::getPixelColor(int indexV) {
  if (mappingTable.size()) {
    if (indexV >= mappingTable.size()) return CRGB::Black;
    if (!mappingTable[indexV].size() || mappingTable[indexV][0] > NUM_LEDS_Preview) return CRGB::Black;

    return ledsP[mappingTable[indexV][0]]; //any would do as they are all the same
  }
  else //no projection
    return ledsP[indexV];
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

