/*
   @title     StarMod
   @file      UserModExample.h
   @date      20230810
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

class UserModExample:public Module {

public:

  UserModExample() :Module("Usermod example") {
    USER_PRINTF("%s %s\n", __PRETTY_FUNCTION__, name);

    USER_PRINTF("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };

  //setup filesystem
  void setup() {
    Module::setup();
    USER_PRINTF("%s %s\n", __PRETTY_FUNCTION__, name);

    USER_PRINTF("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
  }

};

static UserModExample *example;