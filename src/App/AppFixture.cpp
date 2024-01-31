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

      for (std::vector<Leds>::iterator leds=ledsList.begin(); leds!=ledsList.end(); leds++) {
        //vectors really gone now?
        for (std::vector<std::vector<uint16_t>> ::iterator physMap=leds->mappingTable.begin(); physMap!=leds->mappingTable.end(); physMap++)
          physMap->clear();
        leds->mappingTable.clear();
      }
      ledCounter = 0;

      //deallocate all led pins
      uint8_t pinNr = 0;
      for (PinObject pinObject: pins->pinObjects) {
        if (strcmp(pinObject.owner, "Leds") == 0)
          pins->deallocatePin(pinNr, "Leds");
        pinNr++;
      }

      prevLeds = 0;

      // //pre-processing
      // switch(projectionNr) {
      //   case p_None:
      //     break;
      //   case p_Random:
      //     break;
      //   case p_DistanceFromPoint:
      //     // startPos.x = 0;
      //     // startPos.y = 0;
      //     // startPos.z = 0;
      //     break;
      //   case p_DistanceFromCenter:
      //     // startPos.x = size.x / 2;
      //     // startPos.y = size.y / 2;
      //     // startPos.z = size.z / 2;
      //     break;
      // }

      //what to deserialize
      jrdws.lookFor("width", &size.x);
      jrdws.lookFor("height", &size.y);
      jrdws.lookFor("depth", &size.z);
      jrdws.lookFor("nrOfLeds", &nrOfLeds);
      jrdws.lookFor("pin", &currPin);

      //lookFor leds array and for each item in array call lambdo to make a projection
      jrdws.lookFor("leds", [this](std::vector<uint16_t> uint16CollectList) { //this will be called for each tuple of coordinates!
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
          for (std::vector<Leds>::iterator leds=ledsList.begin(); leds!=ledsList.end(); leds++) {
            if (pixel >= leds->startPos && pixel <=leds->endPos) {
              // USER_PRINTF(" XXX %d %d %d", leds->projectionNr, leds->effectDimension, fixtureDimension);

              // USER_PRINTF("projectionNr p:%d f:%d s:%d, %d-%d-%d %d-%d-%d %d-%d-%d\n", projectionNr, effectDimension, fixtureDimension, x, y, z, uint16CollectList[0], uint16CollectList[1], uint16CollectList[2], size.x, size.y, size.z);

              //calculate the bucket to add to current physical led to
              uint16_t bucket = UINT16_MAX;
              switch(leds->projectionNr) {
                case p_None:
                  break;
                case p_Random:
                  break;
                case p_DistanceFromPoint:
                case p_DistanceFromCenter:
                  if (leds->effectDimension == _1D) {
                    if (fixtureDimension == _1D)
                      bucket = distance(pixel.x,0, 0, leds->startPos.x,0,0);
                    else if (fixtureDimension == _2D) { 
                      bucket = distance(pixel.x,pixel.y,0, leds->startPos.x, leds->startPos.y,0);
                      // USER_PRINTF("bucket %d-%d %d-%d %d\n", x,y, startPos.x, startPos.y, bucket);
                    }
                    else if (fixtureDimension == _3D)
                      bucket = distance(pixel, leds->startPos);
                  }
                  else if (leds->effectDimension == _2D) {
                    leds->size.z = 1; //no 3D
                    if (fixtureDimension == _1D)
                      bucket = pixel.x;
                    else if (fixtureDimension == _2D) {
                      leds->size.x = abs(leds->endPos.x - leds->startPos.x + 1);
                      leds->size.y = abs(leds->endPos.y - leds->startPos.y + 1);
                      leds->size.z = 1;

                      pixel -= leds->startPos;

                      //scaling (check rounding errors)
                      //1024 crash in makebuffer...
                      float scale = 1;
                      if (leds->size.x * leds->size.y > 256)
                        scale = (sqrt((float)256.0 / (leds->size.x * leds->size.y))); //avoid high virtual resolutions
                      leds->size.x *= scale;
                      leds->size.y *= scale;
                      pixel.x = (pixel.x+1) * scale - 1;
                      pixel.y = (pixel.y+1) * scale - 1;
                      pixel.z = 0;

                      bucket = leds->XYZ(pixel);
                      // USER_PRINTF("2D to 2D bucket %f %d  %d x %d %d x %d\n", scale, bucket, x, y, size.x, size.y);
                    }
                    else if (fixtureDimension == _3D) {
                      leds->size.x = size.x + size.y;
                      leds->size.y = size.z;
                      leds->size.z = 1;
                      bucket = leds->XY(pixel.x + pixel.y + 1, pixel.z);
                      // USER_PRINTF("2D to 3D bucket %d %d\n", bucket, size.x);
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
                    leds->size.z = 1; //no 3D
                    if (fixtureDimension == _2D) {

                      float xNew = sin(pixel.x * TWO_PI / (float)(size.x-1)) * size.x;
                      float yNew = cos(pixel.x * TWO_PI / (float)(size.x-1)) * size.y;

                      xNew = round(((size.y-1.0-pixel.y)/(size.y-1.0) * xNew + size.x) / 2.0);
                      yNew = round(((size.y-1.0-pixel.y)/(size.y-1.0) * yNew + size.y) / 2.0);

                      USER_PRINTF(" %d,%d->%f,%f->%f,%f", pixel.x, pixel.y, sin(pixel.x * TWO_PI / (float)(size.x-1)), cos(pixel.x * TWO_PI / (float)(size.x-1)), xNew, yNew);
                      pixel.x = xNew;
                      pixel.y = yNew;
                      pixel.z = 0;

                      leds->size.x = size.x;
                      leds->size.y = size.y;
                      leds->size.z = 1;

                      bucket = leds->XYZ(pixel);
                      // USER_PRINTF("2D to 2D bucket %f %d  %d x %d %d x %d\n", scale, bucket, x, y, size.x, size.y);
                    }
                  }
                  break;
              }

              if (bucket != UINT16_MAX) {
                //post processing: inverse mapping
                switch(leds->projectionNr) {
                case p_DistanceFromCenter:
                  switch (leds->effectDimension) {
                  case _2D: 
                    switch (fixtureDimension) {
                    case _2D: 
                      float minDistance = 10;
                      // USER_PRINTF("checking bucket %d\n", bucket);
                      for (uint16_t y=0; y<size.y && minDistance > 0.5; y++)
                      for (uint16_t x=0; x<size.x && minDistance > 0.5; x++) {

                        float xNew = sin(x * TWO_PI / (float)(size.x-1)) * size.x;
                        float yNew = cos(x * TWO_PI / (float)(size.x-1)) * size.y;

                        xNew = round(((size.y-1.0-y)/(size.y-1.0) * xNew + size.x) / 2.0);
                        yNew = round(((size.y-1.0-y)/(size.y-1.0) * yNew + size.y) / 2.0);

                        // USER_PRINTF(" %d,%d->%f,%f->%f,%f", x, y, sin(x * TWO_PI / (float)(size.x-1)), cos(x * TWO_PI / (float)(size.x-1)), xNew, yNew);

                        float distance = abs(bucket - xNew - yNew * size.x);

                        //this should work (better) but needs more testing
                        // if (distance < minDistance) {
                        //   minDistance = distance;
                        //   bucket = x+y*size.x;
                        // }

                        if (bucket == (uint8_t)xNew + (uint8_t)yNew * size.x) {
                          // USER_PRINTF("  found one %d => %d=%d+%d*%d (%f+%f*%d) [%f]\n", bucket, x+y*size.x, x,y, size.x, xNew, yNew, size.x, distance);
                          bucket = leds->XY(x, y);
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

                if (bucket != UINT16_MAX) { //can be nulled by inverse mapping 
                  //add physical tables if not present
                  if (bucket >= NUM_LEDS_Max) {
                    USER_PRINTF("mapping add physMap %d>=%d (%d) too big %d\n", bucket, NUM_LEDS_Max, leds->mappingTable.size(), UINT16_MAX);
                  }
                  else {
                    //create new physMaps if needed
                    if (bucket >= leds->mappingTable.size()) {
                      for (int i = leds->mappingTable.size(); i<=bucket;i++) {
                        // USER_PRINTF("mapping add physMap %d %d\n", bucket, mappingTable.size());
                        std::vector<uint16_t> physMap;
                        leds->mappingTable.push_back(physMap);
                      }
                    }
                    leds->mappingTable[bucket].push_back(ledCounter); //add the current led in the right physMap
                  }
                }
                // USER_PRINTF("mapping b:%d t:%d V:%d\n", bucket, ledCounter, leds->mappingTable.size());
              } //bucket

              // delay(1); //feed the watchdog
            } //if x,y,z between start and endpos
          } //ledsList
          ledCounter++; //also increase if no buffer created
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
              print->fFormat(details, sizeof(details)-1, "%d-%d", min(prevLeds, startLed), max((uint16_t)(ledCounter - 1), nrOfLeds)); //careful: AppModLeds:loop uses this to assign to FastLed
              USER_PRINTF("pins extend leds %d: %s\n", currPin, details);
              //tbd: more check

              strncpy(pins->pinObjects[currPin].details, details, sizeof(PinObject::details)-1);  
            }
          }
          else {//allocate new pin
            //tbd: check if free
            print->fFormat(details, sizeof(details)-1, "%d-%d", prevLeds, ledCounter - 1); //careful: AppModLeds:loop uses this to assign to FastLed
            USER_PRINTF("pins %d: %s\n", currPin, details);
            pins->allocatePin(currPin, "Leds", details);
          }

          prevLeds = ledCounter;
        }
      }); //create the right type, otherwise crash

      if (jrdws.deserialize(false)) { //this will call above function parameter for each led

        uint8_t rowNr = 0;
        for (std::vector<Leds>::iterator leds=ledsList.begin(); leds!=ledsList.end(); leds++) {
          if (leds->projectionNr <= p_Random) {
            //defaults
            leds->size = size;
            leds->nrOfLeds = nrOfLeds;
          }

          if (leds->projectionNr > p_Random) {
            leds->nrOfLeds = leds->mappingTable.size();

            // uint16_t x=0;
            // uint16_t y=0;
            // for (std::vector<uint16_t>physMap:leds->mappingTable) {
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

          USER_PRINTF("projectAndMap V:%dx%dx%d V:%dx%dx%d and P:%d P:%d\n", leds->size.x, leds->size.y, leds->size.z, size.x, size.y, size.z, leds->nrOfLeds, nrOfLeds);
          mdl->setValue("fxSize", leds->size, rowNr);
          mdl->setValue("fxCount", leds->nrOfLeds, rowNr);
          rowNr++;
        }

        mdl->setValue("fixSize", size);
        mdl->setValue("fixCount", nrOfLeds);

      } // if deserialize
    } //if fileName
    else
      USER_PRINTF("projectAndMap: Filename for fixture %d not found\n", fixtureNr);
  }

