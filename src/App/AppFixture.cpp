/*
   @title     StarMod
   @file      AppFixture.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "AppFixture.h"

#include "../Sys/SysModFiles.h"
#include "../Sys/SysJsonRDWS.h"
#include "../Sys/SysModPins.h"


  //load fixture json file, parse it and depending on the projection, create a mapping for it
  void Fixture::projectAndMap() {
    char fileName[32] = "";

    if (files->seqNrToName(fileName, fixtureNr)) {
      JsonRDWS jrdws(fileName); //open fileName for deserialize

      for (std::vector<Leds>::iterator leds=ledsList.begin(); leds!=ledsList.end() && leds->doMap; ++leds) {
        //vectors really gone now?
        for (std::vector<std::vector<uint16_t>> ::iterator physMap=leds->mappingTable.begin(); physMap!=leds->mappingTable.end(); ++physMap)
          physMap->clear();
        leds->mappingTable.clear();
      }

      //deallocate all led pins
      uint8_t pinNr = 0;
      for (PinObject pinObject: pins->pinObjects) {
        if (strcmp(pinObject.owner, "Leds") == 0)
          pins->deallocatePin(pinNr, "Leds");
        pinNr++;
      }

      uint16_t indexP = 0;
      uint16_t prevIndexP = 0;
      uint8_t currPin;

      //what to deserialize
      jrdws.lookFor("width", &size.x);
      jrdws.lookFor("height", &size.y);
      jrdws.lookFor("depth", &size.z);
      jrdws.lookFor("nrOfLeds", &nrOfLeds);
      jrdws.lookFor("pin", &currPin);

      //lookFor leds array and for each item in array call lambdo to make a projection
      jrdws.lookFor("leds", [this, &prevIndexP, &indexP, &currPin](std::vector<uint16_t> uint16CollectList) { //this will be called for each tuple of coordinates!
        // USER_PRINTF("funList ");
        // for (uint16_t num:uint16CollectList)
        //   USER_PRINTF(" %d", num);
        // USER_PRINTF("\n");

        uint8_t fixtureDimension = 0;
        if (size.x>1) fixtureDimension++;
        if (size.y>1) fixtureDimension++;
        if (size.z>1) fixtureDimension++;

        if (uint16CollectList.size()>=1 && fixtureDimension>=1 && fixtureDimension<=3) {

          Coord3D pixel;
          pixel.x = uint16CollectList[0] / 10;
          pixel.y = (fixtureDimension>=2)?uint16CollectList[1] / 10 : 0;
          pixel.z = (fixtureDimension>=3)?uint16CollectList[2] / 10 : 0;

          // USER_PRINTF("led %d,%d,%d start %d,%d,%d end %d,%d,%d\n",x,y,z, startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z);

          // for (Leds leds:ledsList) {
          //vector iterator needed to get the pointer to leds as we need to update leds, also vector iteration on classes is faster!!!
          //search: ^(?=.*\bfor\b)(?=.*\b:\b).*$
          for (std::vector<Leds>::iterator leds=ledsList.begin(); leds!=ledsList.end() && leds->doMap; ++leds) {
            Coord3D startPosAdjusted = (leds->startPos).minimum(size - Coord3D{1,1,1}) ;
            Coord3D endPosAdjusted = (leds->endPos).minimum(size - Coord3D{1,1,1}) ;

            if (pixel >= startPosAdjusted && pixel <= endPosAdjusted) {

              // to display ScrollingText on one side of a cube (WIP)
              if (fixtureDimension == 3 && endPosAdjusted.z == 0)
                fixtureDimension = _2D;

              float scale = 1;
              switch (leds->effectDimension) {
                case _1D:
                  leds->size.x = leds->mappingTable.size() + 1;
                  leds->size.y = 1;
                  leds->size.z = 1;
                  break;
                case _2D:
                  leds->size.x = abs(endPosAdjusted.x - startPosAdjusted.x) + 1;
                  leds->size.y = abs(endPosAdjusted.y - startPosAdjusted.y) + 1;
                  leds->size.z = 1;
                  
                  //scaling (check rounding errors)
                  //1024 crash in makebuffer...
                  // if (leds->size.x * leds->size.y > 256)
                  //   scale = (sqrt((float)256.0 / (leds->size.x * leds->size.y))); //avoid high virtual resolutions
                  // leds->size.x *= scale;
                  // leds->size.y *= scale;
                  break;
              }

              // if (leds->fx == 11) { //lines2D
              //   // USER_PRINTF(" XXX %d %d %d %d, %d, %d", leds->projectionNr, leds->effectDimension, fixtureDimension, pixel.x, pixel.y, pixel.z);
              //   USER_PRINTF(" %d: %d,%d,%d", indexP, pixel.x, pixel.y, pixel.z);
              // }

              // USER_PRINTF("projectionNr p:%d f:%d s:%d, %d-%d-%d %d-%d-%d %d-%d-%d\n", projectionNr, effectDimension, fixtureDimension, x, y, z, uint16CollectList[0], uint16CollectList[1], uint16CollectList[2], size.x, size.y, size.z);

              //calculate the indexV to add to current physical led to
              uint16_t indexV = UINT16_MAX;
              switch(leds->projectionNr) {
                case p_None:
                  break;
                case p_Random:
                  break;
                case p_DistanceFromPoint:
                case p_DistanceFromCenter:
                  if (leds->effectDimension == _1D) {
                    if (fixtureDimension == _1D)
                      indexV = distance(pixel.x,0, 0, startPosAdjusted.x,0,0);
                    else if (fixtureDimension == _2D) { 
                      indexV = distance(pixel.x,pixel.y,0, startPosAdjusted.x, startPosAdjusted.y,0);
                      // USER_PRINTF("indexV %d-%d %d-%d %d\n", x,y, startPos.x, startPos.y, indexV);
                    }
                    else if (fixtureDimension == _3D) {
                      indexV = distance(pixel, startPosAdjusted);
                      // USER_PRINTF(" %d,%d,%d - %d,%d,%d -> %d",pixel.x,pixel.y,pixel.z, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, indexV);
                    }
                  }
                  else if (leds->effectDimension == _2D) {
                    if (fixtureDimension == _1D)
                      indexV = pixel.x;
                    else if (fixtureDimension == _2D) {

                      Coord3D newPixel = pixel - startPosAdjusted;

                      newPixel.x = (newPixel.x+1) * scale - 1;
                      newPixel.y = (newPixel.y+1) * scale - 1;
                      newPixel.z = 0;

                      indexV = leds->XYZ(newPixel);
                      // USER_PRINTF("2D to 2D indexV %f %d  %d x %d %d x %d\n", scale, indexV, x, y, size.x, size.y);
                    }
                    else if (fixtureDimension == _3D) {
                      leds->size.x = size.x + size.y;
                      leds->size.y = size.z;
                      indexV = leds->XY(pixel.x + pixel.y + 1, pixel.z);
                      // USER_PRINTF("2D to 3D indexV %d %d\n", indexV, size.x);
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
                  if (leds->effectDimension == _2D) {
                    if (fixtureDimension == _2D) {

                      float xNew = sin(pixel.x * TWO_PI / (float)(size.x-1)) * size.x;
                      float yNew = cos(pixel.x * TWO_PI / (float)(size.x-1)) * size.y;

                      xNew = round(((size.y-1.0-pixel.y)/(size.y-1.0) * xNew + size.x) / 2.0);
                      yNew = round(((size.y-1.0-pixel.y)/(size.y-1.0) * yNew + size.y) / 2.0);

                      USER_PRINTF(" %d,%d->%f,%f->%f,%f", pixel.x, pixel.y, sin(pixel.x * TWO_PI / (float)(size.x-1)), cos(pixel.x * TWO_PI / (float)(size.x-1)), xNew, yNew);

                      indexV = leds->XYZ({(uint16_t)xNew, (uint16_t)yNew, 0});
                      // USER_PRINTF("2D to 2D indexV %f %d  %d x %d %d x %d\n", scale, indexV, x, y, size.x, size.y);
                    }
                  }
                  break;
              }
              leds->nrOfLeds = leds->size.x * leds->size.y * leds->size.z;

              if (indexV > leds->nrOfLeds) {
                USER_PRINTF("indexV too high %d>=%d (p:%d) p:%d,%d,%d\n", indexV, leds->nrOfLeds, indexP, pixel.x, pixel.y, pixel.z);
              }
              else if (indexV != UINT16_MAX) {
                //post processing: inverse mapping
                switch(leds->projectionNr) {
                case p_DistanceFromCenter:
                  switch (leds->effectDimension) {
                  case _2D: 
                    switch (fixtureDimension) {
                    case _2D: 
                      float minDistance = 10;
                      // USER_PRINTF("checking indexV %d\n", indexV);
                      for (uint16_t y=0; y<size.y && minDistance > 0.5; y++)
                      for (uint16_t x=0; x<size.x && minDistance > 0.5; x++) {

                        float xNew = sin(x * TWO_PI / (float)(size.x-1)) * size.x;
                        float yNew = cos(x * TWO_PI / (float)(size.x-1)) * size.y;

                        xNew = round(((size.y-1.0-y)/(size.y-1.0) * xNew + size.x) / 2.0);
                        yNew = round(((size.y-1.0-y)/(size.y-1.0) * yNew + size.y) / 2.0);

                        // USER_PRINTF(" %d,%d->%f,%f->%f,%f", x, y, sin(x * TWO_PI / (float)(size.x-1)), cos(x * TWO_PI / (float)(size.x-1)), xNew, yNew);

                        float distance = abs(indexV - xNew - yNew * size.x);

                        //this should work (better) but needs more testing
                        // if (distance < minDistance) {
                        //   minDistance = distance;
                        //   indexV = x+y*size.x;
                        // }

                        if (indexV == (uint8_t)xNew + (uint8_t)yNew * size.x) {
                          // USER_PRINTF("  found one %d => %d=%d+%d*%d (%f+%f*%d) [%f]\n", indexV, x+y*size.x, x,y, size.x, xNew, yNew, size.x, distance);
                          indexV = leds->XY(x, y);
                          minDistance = 0; // stop looking further
                        }
                      }
                      if (minDistance > 0.5) indexV = -1;
                      break;
                    }
                    break;
                  }
                  break;
                }

                if (indexV > leds->nrOfLeds) {
                  USER_PRINTF("indexV too high %d>=%d (p:%d) p:%d,%d,%d\n", indexV, leds->nrOfLeds, indexP, pixel.x, pixel.y, pixel.z);
                }
                else if (indexV != UINT16_MAX) { //can be nulled by inverse mapping 
                  //add physical tables if not present
                  if (indexV >= NUM_LEDS_Max / 2) {
                    USER_PRINTF("dev mapping add physMap %d>=%d/2 (V:%d) too big\n", indexV, NUM_LEDS_Max, leds->mappingTable.size());
                  }
                  else {
                    //create new physMaps if needed
                    if (indexV >= leds->mappingTable.size()) {
                      for (int i = leds->mappingTable.size(); i<=indexV;i++) {
                        // USER_PRINTF("mapping %d,%d,%d add physMap before %d %d\n", pixel.y, pixel.y, pixel.z, indexV, leds->mappingTable.size());
                        std::vector<uint16_t> physMap;
                        leds->mappingTable.push_back(physMap); //abort() was called at PC 0x40191473 on core 1 std::allocator<unsigned short> >&&)
                      }
                    }
                    //indexV is within the square
                    if (indexP < NUM_LEDS_Max) leds->mappingTable[indexV].push_back(indexP); //add the current led in the right physMap
                  }
                }
                // USER_PRINTF("mapping b:%d t:%d V:%d\n", indexV, indexP, leds->mappingTable.size());
              } //indexV

              // delay(1); //feed the watchdog
            } //if x,y,z between start and endpos
          } //ledsList
          indexP++; //also increase if no buffer created
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
              print->fFormat(details, sizeof(details)-1, "%d-%d", min(prevIndexP, startLed), max((uint16_t)(indexP - 1), nrOfLeds)); //careful: AppModLeds:loop uses this to assign to FastLed
              USER_PRINTF("pins extend leds %d: %s\n", currPin, details);
              //tbd: more check

              strncpy(pins->pinObjects[currPin].details, details, sizeof(PinObject::details)-1);  
            }
          }
          else {//allocate new pin
            //tbd: check if free
            print->fFormat(details, sizeof(details)-1, "%d-%d", prevIndexP, indexP - 1); //careful: AppModLeds:loop uses this to assign to FastLed
            USER_PRINTF("pins %d: %s\n", currPin, details);
            pins->allocatePin(currPin, "Leds", details);
          }

          prevIndexP = indexP;
        }
      }); //create the right type, otherwise crash

      if (jrdws.deserialize(false)) { //this will call above function parameter for each led

        uint8_t rowNr = 0;
        for (std::vector<Leds>::iterator leds=ledsList.begin(); leds!=ledsList.end() && leds->doMap; ++leds) {
          USER_PRINTF("leds loop %d %d\n", leds->rowNr, leds->fx);
          if (leds->projectionNr <= p_Random) {
            //defaults
            leds->size = size;
            leds->nrOfLeds = nrOfLeds;
          }

          else if (leds->projectionNr > p_Random) {

            if (leds->mappingTable.size() < leds->size.x * leds->size.y * leds->size.z)
              USER_PRINTF("mapping add extra physMap %d of %d %d,%d,%d\n", leds->mappingTable.size(), leds->size.x * leds->size.y * leds->size.z, leds->size.x, leds->size.y, leds->size.z);
            for (int i = leds->mappingTable.size(); i<min(leds->size.x * leds->size.y * leds->size.z, 2048);i++) {
              std::vector<uint16_t> physMap;
              leds->mappingTable.push_back(physMap);
            }
            leds->nrOfLeds = leds->mappingTable.size();

            // uint16_t x=0; //indexV
            // for (std::vector<std::vector<uint16_t>>::iterator physMap=leds->mappingTable.begin(); physMap!=leds->mappingTable.end(); ++physMap) {
            //   if (physMap->size()) {
            //     USER_PRINTF("ledV %d mapping: firstLedP: %d #ledsP: %d", x, physMap[0], physMap->size());
            //     for (uint16_t indexP:*physMap) {USER_PRINTF(" %d", indexP);}
            //     USER_PRINTF("\n");
            //   }
            //   // else
            //   //   USER_PRINTF("ledV %d no mapping\n", x);
            //   x++;
            // }
          }

          USER_PRINTF("projectAndMap V:%dx%dx%d  = %d\n", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);

          mdl->setValueV("fxSize", leds->rowNr, "%d x %d x %d = %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);

          USER_PRINTF("leds[%d].size = %d + %d\n", leds->rowNr, sizeof(Leds), leds->mappingTable.size()); //44

          rowNr++;
          leds->doMap = false;
        } // leds

        USER_PRINTF("projectAndMap P:%dx%dx%d = %d\n", size.x, size.y, size.z, nrOfLeds);

        mdl->setValue("fixSize", size);
        mdl->setValue("fixCount", nrOfLeds);

      } // if deserialize
    } //if fileName
    else
      USER_PRINTF("projectAndMap: Filename for fixture %d not found\n", fixtureNr);

    doMap = false;
  }