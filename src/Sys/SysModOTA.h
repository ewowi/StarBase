/*
   @title     StarMod
   @file      SysModOTA.h
   @date      20231111
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModule.h"

#include <ArduinoOTA.h>

class SysModOTA : public SysModule {

public:
    SysModOTA() : SysModule("OTA") {
        this->success = true;
        this->started = false;
    };

    void connectedChanged() {
        if(!SysModules::isConnected) return;
        ArduinoOTA.begin();
        this->started = true;
        USER_PRINTF("ArduinoOTA.begin()\n");
     }

    void loop() {
        if(this->started) {
            ArduinoOTA.handle();
        }
    }
    
    void loop10s() {
        if(!this->started) {
            USER_PRINTF("ArduinoOTA not started\n");
        }
    }

private: 
    bool started;
};

static SysModOTA *ota;