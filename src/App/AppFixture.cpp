/*
   @title     StarMod
   @file      AppFixture.cpp
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "AppFixture.h"

#include "../Sys/SysModFiles.h"
#include "../Sys/SysStarModJson.h"
#include "../Sys/SysModPins.h"

Coord3D map1Dto2D(Coord3D in) {
  Coord3D out;
  out.x = sqrt(in.x * in.y * in.z); //only one is > 1, square root
  out.y = out.x;
  out.z = 0;
  return out;
}
Coord3D map2Dto2D(Coord3D in) {
  Coord3D out;
  out.x = in.x;//in.x>1?in.x:in.y; //2 of the 3 sizes are > 1
  out.y = in.y;//in.x>1?in.y:in.z;
  out.z = in.z;//0;
  return out;
}


//load fixture json file, parse it and depending on the projection, create a mapping for it
void Fixture::projectAndMap() {
  char fileName[32] = "";

  if (files->seqNrToName(fileName, fixtureNr)) {
    StarModJson starModJson(fileName); //open fileName for deserialize

    // for (std::vector<Leds *>::iterator leds=ledsList.begin(); leds!=ledsList.end() && leds->doMap; ++leds) {
    unsigned8 rowNr = 0;
    for (Leds *leds: ledsList) {
      if (leds->doMap) {
        USER_PRINTF("Leds pre [%d] f:%d p:%d s:%d\n", rowNr, leds->fx, leds->projectionNr, ledsList.size());
        //vectors really gone now?
        for (std::vector<std::vector<unsigned16>> ::iterator physMap=leds->mappingTable.begin(); physMap!=leds->mappingTable.end(); ++physMap)
          physMap->clear();
        leds->mappingTable.clear();
        leds->sharedData.clear();
      }
      rowNr++;
    }

    //deallocate all led pins
    unsigned8 pinNr = 0;
    for (PinObject pinObject: pins->pinObjects) {
      if (strcmp(pinObject.owner, "Leds") == 0)
        pins->deallocatePin(pinNr, "Leds");
      pinNr++;
    }

    unsigned16 indexP = 0;
    unsigned16 prevIndexP = 0;
    unsigned16 currPin;

    //what to deserialize
    starModJson.lookFor("width", &size.x);
    starModJson.lookFor("height", &size.y);
    starModJson.lookFor("depth", &size.z);
    starModJson.lookFor("nrOfLeds", &nrOfLeds);
    starModJson.lookFor("pin", &currPin);

    //lookFor leds array and for each item in array call lambdo to make a projection
    starModJson.lookFor("leds", [this, &prevIndexP, &indexP, &currPin](std::vector<unsigned16> uint16CollectList) { //this will be called for each tuple of coordinates!
      // USER_PRINTF("funList ");
      // for (unsigned16 num:uint16CollectList)
      //   USER_PRINTF(" %d", num);
      // USER_PRINTF("\n");

      if (uint16CollectList.size()>=1) { // a pixel

        Coord3D pixel; //in mm !
        pixel.x = uint16CollectList[0];
        pixel.y = (uint16CollectList.size()>=2)?uint16CollectList[1]: 0;
        pixel.z = (uint16CollectList.size()>=3)?uint16CollectList[2]: 0;

        // USER_PRINTF("led %d,%d,%d start %d,%d,%d end %d,%d,%d\n",x,y,z, startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z);

        //vector iterator needed to get the pointer to leds as we need to update leds, also vector iteration on classes is faster!!!
        //search: ^(?=.*\bfor\b)(?=.*\b:\b).*$
        unsigned8 rowNr = 0;
        for (Leds *leds: ledsList) {
          if (leds->doMap) {

            //set start and endPos between bounderies of fixture
            Coord3D startPosAdjusted = (leds->startPos).minimum(size - Coord3D{1,1,1}) * 10;
            Coord3D endPosAdjusted = (leds->endPos).minimum(size - Coord3D{1,1,1}) * 10;
            Coord3D projSize = (endPosAdjusted - startPosAdjusted)/10 + Coord3D{1,1,1};

            // 0 to 3D depending on start and endpos (e.g. to display ScrollingText on one side of a cube)
            unsigned8 projectionDimension = 0;
            if (projSize.x > 1) projectionDimension++;
            if (projSize.y > 1) projectionDimension++;
            if (projSize.z > 1) projectionDimension++;

            // mdl->setValue("fxStart", startPosAdjusted/10, rowNr); //rowNr
            // mdl->setValue("fxEnd", endPosAdjusted/10, rowNr); //rowNr

            //only needed one time
            //does not work for some reason...

            // if (indexP == 0) //first
            {
              unsigned16 maxDistance = distance(endPosAdjusted, startPosAdjusted) / 10;
              // USER_PRINTF("maxDistance %d %d,%d,%d %d,%d,%d %d,%d,%d\n", maxDistance, pixel.x, pixel.y, pixel.z, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, endPosAdjusted.x, endPosAdjusted.y, endPosAdjusted.z);

              float scale = 1;
              switch (leds->effectDimension) {
                case _1D:
                  leds->size.x = maxDistance;
                  leds->size.y = 1;
                  leds->size.z = 1;
                  break;
                case _2D:
                  switch(projectionDimension) {
                    case _1D:
                      leds->size = map1Dto2D(projSize);
                      // leds->size.x = sqrt(projSize.x * projSize.y * projSize.z); //only one is > 1, square root
                      // leds->size.y = leds->size.x;
                      break;
                    case _2D:
                      if (leds->projectionNr == p_Multiply) {
                        Coord3D split = mdl->getValue("proSplit", rowNr);
                        leds->size = map2Dto2D(projSize) / split;
                        // USER_PRINTF("Multiply %d,%d,%d\n", leds->size.x, leds->size.y, leds->size.z);
                      }
                      else 
                        leds->size = map2Dto2D(projSize);
                      break;
                    case _3D:
                      leds->size.x = projSize.x + projSize.y;
                      leds->size.y = projSize.z;
                      leds->size.z = 1;
                      break;
                  }
                  break;
                case _3D:
                  switch(projectionDimension) {
                    case _1D:
                      leds->size.x = std::pow(projSize.x * projSize.y * projSize.z, 1/3.); //only one is > 1, cube root
                      leds->size.y = leds->size.x;
                      leds->size.z = leds->size.x;
                      break;
                    case _2D:
                      leds->size.x = projSize.x; //2 of the 3 sizes are > 1, so one dimension of the effect is 1
                      leds->size.y = projSize.y;
                      leds->size.z = projSize.z;
                      break;
                    case _3D:
                      leds->size.x = projSize.x;
                      leds->size.y = projSize.y;
                      leds->size.z = projSize.z;
                      break;
                  }
                  
                  break;
                  //scaling (check rounding errors)
                  //1024 crash in makebuffer...
                  // if (leds->size.x * leds->size.y > 256)
                  //   scale = (sqrt(256.0f / (leds->size.x * leds->size.y))); //avoid high virtual resolutions
                  // leds->size.x *= scale;
                  // leds->size.y *= scale;
              }
              leds->nrOfLeds = leds->size.x * leds->size.y * leds->size.z;
            } //only one time

            if (indexP == 0)
              USER_PRINTF("first [%d] s:%d,%d,%d e:%d,%d,%d s:%d,%d,%d mt:%d\n", rowNr, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, endPosAdjusted.x, endPosAdjusted.y, endPosAdjusted.z, leds->size.x, leds->size.y, leds->size.z, leds->mappingTable.size());

            if (pixel >= startPosAdjusted && pixel <= endPosAdjusted ) { //if pixel between start and end pos

              // if (leds->fx == 11) { //lines2D
              //   // USER_PRINTF(" XXX %d %d %d %d, %d, %d", leds->projectionNr, leds->effectDimension, projectionDimension, pixel.x, pixel.y, pixel.z);
              //   USER_PRINTF(" %d: %d,%d,%d", indexP, pixel.x, pixel.y, pixel.z);
              // }

              // USER_PRINTF("projectionNr p:%d f:%d s:%d, %d-%d-%d %d-%d-%d %d-%d-%d\n", projectionNr, effectDimension, projectionDimension, x, y, z, uint16CollectList[0], uint16CollectList[1], uint16CollectList[2], size.x, size.y, size.z);

              Coord3D midPoint = (startPosAdjusted + endPosAdjusted) / 2;

              //calculate the indexV to add to current physical led to
              unsigned16 indexV = UINT16_MAX;
              switch(leds->projectionNr) {
                case p_None:
                  break;
                case p_Random:
                  break;
                case p_Default:
                case p_Multiply:
                case p_Rotate:
                case p_DistanceFromPoint:
                  if (leds->effectDimension == _1D) {
                    indexV = distance(pixel, startPosAdjusted) / 10;
                    // USER_PRINTF("Distance %d %d,%d,%d %d,%d,%d %d,%d,%d\n", indexV, pixel.x, pixel.y, pixel.z, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, leds->size.x, leds->size.y, leds->size.z);
                    // if (projectionDimension == _1D)
                    //   indexV = distance(pixel.x, 0, 0, startPosAdjusted.x, 0, 0);
                    // else if (projectionDimension == _2D) { 
                    //   indexV = distance(pixel.x, pixel.y, 0, startPosAdjusted.x, startPosAdjusted.y, 0);
                    //   // USER_PRINTF("indexV %d-%d %d-%d %d\n", x,y, startPos.x, startPos.y, indexV);
                    // }
                    // else if (projectionDimension == _3D) {
                    //   indexV = distance(pixel, startPosAdjusted);
                    //   // USER_PRINTF(" %d,%d,%d - %d,%d,%d -> %d",pixel.x,pixel.y,pixel.z, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, indexV);
                    // }
                  }
                  else if (leds->effectDimension == _2D) {
                    if (projectionDimension == _1D)
                      indexV = leds->XYZ(map1Dto2D((pixel - startPosAdjusted)/10));
                    else if (projectionDimension == _2D) {
                      Coord3D mapped = map2Dto2D(pixel - startPosAdjusted);
                      if (leds->projectionNr == p_Multiply) {
                        mapped.x = mapped.x % (leds->size.x*10);
                        mapped.y = mapped.y % (leds->size.y*10);
                        mapped.z = mapped.y % (leds->size.z*10);
                      }
                      indexV = leds->XYZ(mapped/10);
                      // USER_PRINTF("map2Dto2D %d-%d p:%d,%d,%d m:%d,%d,%d\n", indexV, indexP, pixel.x, pixel.y, pixel.z, mapped.x, mapped.y, mapped.z);

                      // indexV = leds->XYZ((pixel.x>1?pixel.x:pixel.y)/10, (pixel.x>1?pixel.y:pixel.z)/10); //2 of the 3 sizes are > 1

                      // Coord3D newPixel = pixel - startPosAdjusted;

                      // newPixel.x = (newPixel.x+1) * scale - 1;
                      // newPixel.y = (newPixel.y+1) * scale - 1;
                      // newPixel.z = 0;

                      // indexV = leds->XYZ(newPixel);
                      // USER_PRINTF("2D to 2D indexV %f %d  %d x %d %d x %d\n", scale, indexV, x, y, size.x, size.y);
                    }
                    else if (projectionDimension == _3D) {
                      leds->size.x = size.x + size.y;
                      leds->size.y = size.z;
                      indexV = leds->XY((pixel.x + pixel.y)/10 + 1, pixel.z/10);
                      // USER_PRINTF("2D to 3D indexV %d %d\n", indexV, size.x);
                    }
                  }
                  //tbd: effect is 3D
                  break;
                case p_Reverse:
                  break;
                case p_Mirror:
                  break;
                case p_Fun: //first attempt for distance from Circle 2D
                  if (leds->effectDimension == _2D) {
                    if (projectionDimension == _2D) {

                      float xNew = sinf(pixel.x * TWO_PI / (float)(size.x-1)) * size.x;
                      float yNew = cosf(pixel.x * TWO_PI / (float)(size.x-1)) * size.y;

                      xNew = round(((size.y-1.0-pixel.y)/(size.y-1.0) * xNew + size.x) / 2.0);
                      yNew = round(((size.y-1.0-pixel.y)/(size.y-1.0) * yNew + size.y) / 2.0);

                      USER_PRINTF(" %d,%d->%f,%f->%f,%f", pixel.x, pixel.y, sinf(pixel.x * TWO_PI / (float)(size.x-1)), cosf(pixel.x * TWO_PI / (float)(size.x-1)), xNew, yNew);

                      indexV = leds->XYZ({(unsigned16)xNew, (unsigned16)yNew, 0});
                      // USER_PRINTF("2D to 2D indexV %f %d  %d x %d %d x %d\n", scale, indexV, x, y, size.x, size.y);
                    }
                  }
                  break;
              }
              leds->nrOfLeds = leds->size.x * leds->size.y * leds->size.z;

              if (indexV != UINT16_MAX) {
                if (indexV >= leds->nrOfLeds || indexV >= NUM_LEDS_Max) {
                  USER_PRINTF("dev pre [%d] indexV too high %d>=%d or %d (m:%d p:%d) p:%d,%d,%d s:%d,%d,%d\n", rowNr, indexV, leds->nrOfLeds, NUM_LEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z, leds->size.x, leds->size.y, leds->size.z);
                }
                else {
                  //post processing: inverse mapping
                  switch(leds->projectionNr) {
                  case p_DistanceFromPoint:
                    switch (leds->effectDimension) {
                    case _2D: 
                      switch (projectionDimension) {
                      case _2D: 
                        float minDistance = 10;
                        // USER_PRINTF("checking indexV %d\n", indexV);
                        for (unsigned16 y=0; y<size.y && minDistance > 0.5; y++)
                        for (unsigned16 x=0; x<size.x && minDistance > 0.5; x++) {

                          float xNew = sinf(x * TWO_PI / (float)(size.x-1)) * size.x;
                          float yNew = cosf(x * TWO_PI / (float)(size.x-1)) * size.y;

                          xNew = round(((size.y-1.0-y)/(size.y-1.0) * xNew + size.x) / 2.0);
                          yNew = round(((size.y-1.0-y)/(size.y-1.0) * yNew + size.y) / 2.0);

                          // USER_PRINTF(" %d,%d->%f,%f->%f,%f", x, y, sinf(x * TWO_PI / (float)(size.x-1)), cosf(x * TWO_PI / (float)(size.x-1)), xNew, yNew);

                          float distance = abs(indexV - xNew - yNew * size.x);

                          //this should work (better) but needs more testing
                          // if (distance < minDistance) {
                          //   minDistance = distance;
                          //   indexV = x+y*size.x;
                          // }

                          if (indexV == (unsigned8)xNew + (unsigned8)yNew * size.x) {
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

                  if (indexV != UINT16_MAX) { //can be nulled by inverse mapping 
                    //add physical tables if not present
                    if (indexV >= leds->nrOfLeds || indexV >= NUM_LEDS_Max) {
                      USER_PRINTF("dev post [%d] indexV too high %d>=%d or %d (p:%d m:%d) p:%d,%d,%d\n", rowNr, indexV, leds->nrOfLeds, NUM_LEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z);
                    }
                    else if (indexP < NUM_LEDS_Max) {
                      //create new physMaps if needed
                      if (indexV >= leds->mappingTable.size()) {
                        for (size_t i = leds->mappingTable.size(); i <= indexV; i++) {
                          // USER_PRINTF("mapping %d,%d,%d add physMap before %d %d\n", pixel.y, pixel.y, pixel.z, indexV, leds->mappingTable.size());
                          std::vector<unsigned16> physMap;
                          physMap.push_back(0);
                          leds->mappingTable.push_back(physMap); //abort() was called at PC 0x40191473 on core 1 std::allocator<unsigned short> >&&)
                        }
                      }
                      //indexV is within the square
                       leds->mappingTable[indexV].push_back(indexP); //add the current led in the right physMap
                    }
                    else 
                      USER_PRINTF("dev post [%d] indexP too high %d>=%d or %d (p:%d m:%d) p:%d,%d,%d\n", rowNr, indexP, nrOfLeds, NUM_LEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z);
                  }
                  // USER_PRINTF("mapping b:%d t:%d V:%d\n", indexV, indexP, leds->mappingTable.size());
                } //indexV not too high
              } //indexV

              // delay(1); //feed the watchdog
            } //if x,y,z between start and endpos
          } //leds->doMap
          rowNr++;
        } //ledsList
        indexP++; //also increase if no buffer created
      } //if 1D-3D
      else { // end of leds array

        if (doAllocPins) {
          //check if pin already allocated, if so, extend range in details
          PinObject pinObject = pins->pinObjects[currPin];
          char details[32] = "";
          if (pins->isOwner(currPin, "Leds")) { //if owner

            char * after = strtok((char *)pinObject.details, "-");
            if (after != NULL ) {
              char * before;
              before = after;
              after = strtok(NULL, " ");
              unsigned16 startLed = atoi(before);
              unsigned16 nrOfLeds = atoi(after) - atoi(before) + 1;
              print->fFormat(details, sizeof(details)-1, "%d-%d", min(prevIndexP, startLed), max((unsigned16)(indexP - 1), nrOfLeds)); //careful: AppModEffects:loop uses this to assign to FastLed
              USER_PRINTF("pins extend leds %d: %s\n", currPin, details);
              //tbd: more check

              strncpy(pins->pinObjects[currPin].details, details, sizeof(PinObject::details)-1);  
            }
          }
          else {//allocate new pin
            //tbd: check if free
            print->fFormat(details, sizeof(details)-1, "%d-%d", prevIndexP, indexP - 1); //careful: AppModEffects:loop uses this to assign to FastLed
            USER_PRINTF("pins %d: %s\n", currPin, details);
            pins->allocatePin(currPin, "Leds", details);
          }

          prevIndexP = indexP;
        }
      }
    }); //starModJson.lookFor("leds" (create the right type, otherwise crash)

    if (starModJson.deserialize(false)) { //this will call above function parameter for each led

      unsigned8 rowNr = 0;
      // for (std::vector<Leds *>::iterator leds=ledsList.begin(); leds!=ledsList.end() && leds->doMap; ++leds) {
      for (Leds *leds: ledsList) {
        if (leds->doMap) {
          USER_PRINTF("Leds pre [%d] f:%d p:%d s:%d\n", rowNr, leds->fx, leds->projectionNr, ledsList.size());

          unsigned16 nrOfMappings = 0;
          unsigned16 nrOfPixels = 0;

          if (leds->projectionNr == p_Random || leds->projectionNr == p_None) {

            //defaults
            leds->size = size;
            leds->nrOfLeds = nrOfLeds;

          } else {

            if (leds->mappingTable.size() < leds->size.x * leds->size.y * leds->size.z)
              USER_PRINTF("mapping add extra physMap %d of %d %d,%d,%d\n", leds->mappingTable.size(), leds->size.x * leds->size.y * leds->size.z, leds->size.x, leds->size.y, leds->size.z);
            for (size_t i = leds->mappingTable.size(); i < leds->size.x * leds->size.y * leds->size.z; i++) {
              std::vector<unsigned16> physMap;
              physMap.push_back(0);
              leds->mappingTable.push_back(physMap);
            }

            leds->nrOfLeds = leds->mappingTable.size();

            unsigned16 indexV = 0;
            for (std::vector<std::vector<unsigned16>>::iterator physMap=leds->mappingTable.begin(); physMap!=leds->mappingTable.end(); ++physMap) {
              if (physMap->size()) {
                // USER_PRINTF("ledV %d mapping: #ledsP (%d):", indexV, physMap->size());

                for (unsigned16 indexP:*physMap) {
                  // USER_PRINTF(" %d", indexP);
                  nrOfPixels++;
                }

                // USER_PRINTF("\n");
              }
              nrOfMappings++;
              // else
              //   USER_PRINTF("ledV %d no mapping\n", x);
              indexV++;
            }
          }

          USER_PRINTF("projectAndMap [%d] V:%d x %d x %d = %d (%d-%d)\n", rowNr, leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds, nrOfMappings, nrOfPixels);

          // mdl->setValueV("fxSize", rowNr, "%d x %d x %d = %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          char buf[32];
          print->fFormat(buf, sizeof(buf)-1,"%d x %d x %d = %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          mdl->setValue("fxSize", JsonString(buf, JsonString::Copied), rowNr);
          // web->sendResponseObject();

          USER_PRINTF("leds[%d].size = %d + %d\n", rowNr, sizeof(Leds), leds->mappingTable.size()); //44

          leds->doMap = false;
        } //leds->doMap
        rowNr++;
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