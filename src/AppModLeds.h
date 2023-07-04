#include "Module.h"

#include "FastLED.h"



class AppModLeds:public Module {

public:

  AppModLeds() :Module("Leds") {};

  void setup();

  void loop();

};

extern AppModLeds *lds;