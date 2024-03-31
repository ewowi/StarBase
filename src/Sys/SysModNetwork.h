/*
   @title     StarMod
   @file      SysModNetwork.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

#include <DNSServer.h>

class SysModNetwork:public SysModule {

public:

  SysModNetwork();

  //setup wifi an async webserver
  void setup();

  void loop1s();

  //tbd: utility function ...
  void prepareHostname(char* hostname, const char *in)
  {
    const char *pC = in;
    uint8_t pos = 0;
    while (*pC && pos < 24) { // while !null and not over length
      if (isalnum(*pC)) {     // if the current char is alpha-numeric append it to the hostname
        hostname[pos] = *pC;
        pos++;
      } else if (*pC == ' ' || *pC == '_' || *pC == '-' || *pC == '+' || *pC == '!' || *pC == '?' || *pC == '*') {
        hostname[pos] = '-';
        pos++;
      }
      // else do nothing - no leading hyphens and do not include hyphens for all other characters.
      pC++;
    }
    //last character must not be hyphen
    if (pos > 5) {
      while (pos > 4 && hostname[pos -1] == '-') pos--;
      hostname[pos] = '\0'; // terminate string (leave at least "wled")
    }
  }

  void handleConnection();
  void initConnection();

  void handleAP();
  void initAP();
  void stopAP();
  
private:
  bool apActive = false;
  unsigned32 lastReconnectAttempt = 0;
  byte apChannel = 1; // 2.4GHz WiFi AP channel (1-13)
  bool isConfirmedConnection = false;
  DNSServer dnsServer;
  byte stacO = 0; //stationCount
};
  
extern SysModNetwork *net;