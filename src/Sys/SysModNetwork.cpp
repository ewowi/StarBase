/*
   @title     StarMod
   @file      SysModNetwork.cpp
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

#include "SysModNetwork.h"
#include "SysModules.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModUI.h"
#include "SysModModel.h"

#include <WiFi.h>

SysModNetwork::SysModNetwork() :SysModule("Network") {};

//setup wifi an async webserver
void SysModNetwork::setup() {
  SysModule::setup();
  USER_PRINT_FUNCTION("%s %s\n", __PRETTY_FUNCTION__, name);

  parentVar = ui->initModule(parentVar, name);
  ui->initText(parentVar, "ssid", "", 32, false);
  ui->initPassword(parentVar, "pw", "", 32, false, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Password");
  });
  ui->initButton(parentVar, "connect", nullptr, false, [](JsonObject var) {
    web->addResponse(var["id"], "comment", "Force reconnect (you loose current connection)");
  }, [this](JsonObject var) {
    forceReconnect = true;
  });
  ui->initText(parentVar, "nwstatus", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Status");
  });
  ui->initText(parentVar, "rssi", nullptr, 32, true, [](JsonObject var) { //uiFun
    web->addResponse(var["id"], "label", "Wifi signal");
  });

  USER_PRINT_FUNCTION("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
}

void SysModNetwork::loop() {
  // SysModule::loop();

  handleConnection();
}

void SysModNetwork::loop1s() {
  mdl->setValueLossy("rssi", "%d dBm", WiFi.RSSI());
}

void SysModNetwork::handleConnection() {
  if (lastReconnectAttempt == 0) { // do this only once
    USER_PRINTF("lastReconnectAttempt == 0\n");
    initConnection();
    return;
  }

  if (forceReconnect) {
    USER_PRINTF("Forcing reconnect.");
    initConnection();
    interfacesInited = false;
    forceReconnect = false;
    // wasConnected = false;
    return;
  }
  if (!(WiFi.localIP()[0] != 0 && WiFi.status() == WL_CONNECTED)) { //!Network.interfacesInited()
    if (interfacesInited) {
      USER_PRINTF("Disconnected!\n");
      interfacesInited = false;
      initConnection();
    }

    if (!apActive && millis() - lastReconnectAttempt > 12000 ) { //&& (!wasConnected || apBehavior == AP_BEHAVIOR_NO_CONN)
      USER_PRINTF("Not connected AP.\n");
      initAP();
    }
  } else if (!interfacesInited) { //newly connected
    mdl->setValueP("nwstatus", "Connected %d", WiFi.localIP()[3]);

    interfacesInited = true;

    SysModules::newConnection = true; // send all modules connect notification

    // shut down AP
    if (apActive) { //apBehavior != AP_BEHAVIOR_ALWAYS
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      apActive = false;
      USER_PRINTF("Access point disabled (handle).\n");
    }
  }
}

void SysModNetwork::initConnection() {

  // ws.onEvent(wsEvent);

  WiFi.disconnect(true);        // close old connections

  lastReconnectAttempt = millis();

  if (!apActive) {
    USER_PRINTF("Access point disabled (init).\n");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
  }

  WiFi.setSleep(!noWifiSleep);
  WiFi.setHostname("StarMod");

  const char * ssid = mdl->getValue("ssid");
  const char * password = mdl->getValue("pw");
  if (ssid && strlen(ssid)>0) {
    char passXXX [32] = "";
    for (int i = 0; i < strlen(password); i++) strncat(passXXX, "*", sizeof(passXXX)-1);
    USER_PRINTF("Connecting to WiFi %s / %s\n", ssid, passXXX);
    WiFi.begin(ssid, password);
  }
  else
    USER_PRINTF("No SSID");
}

void SysModNetwork::initAP() {
  USER_PRINTF("Opening access point %s\n", apSSID);
  WiFi.softAPConfig(IPAddress(4, 3, 2, 1), IPAddress(4, 3, 2, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID, apPass, apChannel, apHide);
  if (!apActive) // start captive portal if AP active
  {
    mdl->setValueP("nwstatus", "AP %s / %s @ %s", apSSID, apPass, WiFi.softAPIP().toString().c_str());

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    apActive = true;

    SysModules::newConnection = true; // send all modules connect notification
  }

}
