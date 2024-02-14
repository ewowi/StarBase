/*
   @title     StarMod
   @file      SysModNetwork.cpp
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModNetwork.h"
#include "SysModules.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModUI.h"
#include "SysModModel.h"

SysModNetwork::SysModNetwork() :SysModule("Network") {};

//setup wifi an async webserver
void SysModNetwork::setup() {
  SysModule::setup();

  parentVar = ui->initSysMod(parentVar, name);
  if (parentVar["o"] > -1000) parentVar["o"] = -2500; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

  // JsonObject tableVar = ui->initTable(parentVar, "wfTbl", nullptr, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { //varFun ro false: create and delete row possible
  //   ui->setLabel(var, "Wifi");
  //   ui->setComment(var, "List of defined and available Wifi APs");
  // });

  ui->initText(parentVar, "ssid", "", 32, false);
  // , nullptr
  // , nullptr, nullptr, 1, [this](JsonObject var, uint8_t rowNr) { //valueFun
  //   var["value"][0] = "";
  // });

  ui->initPassword(parentVar, "pw", "", 32, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Password");
      return true;
    default: return false;
  }});

  ui->initButton(parentVar, "connect", false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setComment(var, "Force reconnect (loose current connection)");
      return true;
    case f_ChangeFun:
      // mdl->doWriteModel = true; //saves the model
      forceReconnect = true;
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "nwstatus", nullptr, 32, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Status");
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "rssi", nullptr, 32, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Wifi signal");
      return true;
    default: return false;
  }});
}

void SysModNetwork::loop() {
  // SysModule::loop();

  handleConnection();
}

void SysModNetwork::loop1s() {
  mdl->setUIValueV("rssi", "%d dBm", WiFi.RSSI());
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
    mdl->setUIValueV("nwstatus", "Connected %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);

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
    #if defined(STARMOD_LOLIN_WIFI_FIX )
      WiFi.setTxPower(WIFI_POWER_8_5dBm );
    #endif
  }
  else
    USER_PRINTF("No SSID");
}

void SysModNetwork::initAP() {
  USER_PRINTF("Opening access point %s\n", apSSID);
  WiFi.softAPConfig(IPAddress(4, 3, 2, 1), IPAddress(4, 3, 2, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID, apPass, apChannel, apHide);
  #if defined(STARMOD_LOLIN_WIFI_FIX )
    WiFi.setTxPower(WIFI_POWER_8_5dBm );
  #endif
  if (!apActive) // start captive portal if AP active
  {
    mdl->setUIValueV("nwstatus", "AP %s / %s @ %s", apSSID, apPass, WiFi.softAPIP().toString().c_str());

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    apActive = true;

    SysModules::newConnection = true; // send all modules connect notification
  }
}