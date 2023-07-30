/*
   @title     StarMod
   @file      AppModLeds.cpp
   @date      20230729
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
uint16_t LedsV::width = 8; 
uint16_t LedsV::height = 8; 
uint16_t LedsV::depth = 1; 

enum Projections
{
  p_None,
  p_Random,
  p_DistanceFromPoint
};

//load ledfix json file, parse it and depending on the projection, create a mapping for it
void LedsV::ledFixProjectAndMap() {
  char fileName[30] = "";

  if (files->seqNrToName(fileName, ledFixNr)) {
    JsonRDWS jrdws(fileName); //open fileName for deserialize

    mappingTableLedCounter = 0;
    mappingTable.clear();

    //what to deserialize
    jrdws.lookFor("width", &width);
    jrdws.lookFor("height", &height);
    jrdws.lookFor("depth", &depth);
    jrdws.lookFor("nrOfLeds", &nrOfLedsP);

    if (projectionNr > p_Random) { //0 and 1 (none, random have no mapping)
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
          if (dist >= mappingTable.size()) {
            for (int i = mappingTable.size(); i<=dist;i++) {
              // print->print("mapping add physMap %d %d\n", dist, mappingTable.size());
              std::vector<uint16_t> physMap;
              mappingTable.push_back(physMap);
            }
          }

          mappingTable[dist].push_back(mappingTableLedCounter++);

          // print->print("mapping %d V:%d P:%d\n", dist, mappingTable.size(), mappingTableLedCounter);

          // delay(1); //feed the watchdog
        }
      }); //create the right type, otherwise crash

    } //projection != 0
    if (jrdws.deserialize()) { //find all the vars

      if (projectionNr > p_Random) {
        nrOfLedsV = mappingTable.size();

        // uint16_t x=0;
        // uint16_t y=0;
        // for (std::vector<uint16_t>physMap:mappingTable) {
        //   print->print("led %d mapping: ", x);
        //   for (uint16_t pos:physMap) {
        //     print->print(" %d", pos);
        //     y++;
        //   }
        //   print->print("\n");
        //   x++;
        // }
      }
      else
        nrOfLedsV = nrOfLedsP;

      print->print("jrdws whd %d %d %d and P:%d V:%d\n", width, height, depth, nrOfLedsP, nrOfLedsV);

      //at page refresh, done before these vars have been initialized...
      mdl->setValueV("dimensions", "%dx%dx%d", ledsV.width, ledsV.height, ledsV.depth);
      mdl->setValueV("nrOfLeds", "P:%d V:%d", nrOfLedsP, nrOfLedsV);

      //send to pview a message to get file filename
      JsonDocument *responseDoc = web->getResponseDoc();
      responseDoc->clear(); //needed for deserializeJson?
      JsonVariant responseVariant = responseDoc->as<JsonVariant>();

      web->addResponse("pview", "file", fileName);
      web->sendDataWs(responseVariant);
      print->printJson("ledfix chFun send ws done", responseVariant); //during server startup this is not send to a client, so client refresh should also trigger this
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

