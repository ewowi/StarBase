class SysModSystem:public Module {

public:

  unsigned long loopCounter = 0;
  JsonObject clientListObject;

  SysModSystem() :Module("System") {};

  void setup() {
    Module::setup();
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);

    ui->initDisplay(parentObject, "upTime", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "comment", "Uptime of board");
    });
    ui->initDisplay(parentObject, "loops");
    ui->initDisplay(parentObject, "heap", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "comment", "Free / Total (largest free)");
    });
    ui->initDisplay(parentObject, "stack");

    ui->initButton(parentObject, "restart", "Restart", nullptr, [](JsonObject object) {  //chFun
      ws.closeAll(1012);
      ESP.restart();
    });

    //should be in SysModWeb...
    web->parentObject = ui->initGroup(web->parentObject, web->name);
    // ui->initDisplay(web->parentObject, "nrOfC", nullptr, [](JsonObject object) { //uiFun
    //   web->addResponse(object, "label", "Nr of clients");
    // });

    clientListObject = ui->initMany(web->parentObject, "clist", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Clients");
      web->addResponse(object, "comment", "List of clients");
      JsonArray rows = web->addResponseArray(object, "many");
      for (auto client:ws.getClients()) {
        // print->print("Client %d %d %s\n", client->id(), client->queueIsFull(), client->remoteIP().toString().c_str());
        JsonArray row = rows.createNestedArray();
        row.add(client->id());
        row.add((char *)client->remoteIP().toString().c_str()); //create a copy!
        row.add(client->queueIsFull());
      }
    });
    ui->initDisplay(clientListObject, "clNr", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Nr");
    });
    ui->initDisplay(clientListObject, "clIp", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "IP");
    });
    ui->initDisplay(clientListObject, "clIsFull", nullptr, [](JsonObject object) { //uiFun
      web->addResponse(object, "label", "Is full");
    });



    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop() {
    // Module::loop();

    loopCounter++;
    if (millis() - secondMillis >= 1000 || !secondMillis) {
      secondMillis = millis();
      ui->setValueV("upTime", "%u s", millis()/1000);
      ui->setValueV("loops", "%lu /s", loopCounter);
      ui->setValueV("heap", "%d / %d (%d) B", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
      ui->setValueV("stack", "%d B", uxTaskGetStackHighWaterMark(NULL));

      //should be in SysModWeb...

      //if something changed in clist
      if (web->clientsChanged) {
        web->clientsChanged = false;

        //replace clist table
        responseDoc.clear(); //needed for deserializeJson?
        responseDoc["uiFun"] = "clist";
        JsonVariant responseVariant = responseDoc.as<JsonVariant>();
        ui->processJson(responseVariant); //this calls uiFun command
        print->printJson("clist change response", responseDoc);
        web->sendDataWs(responseVariant);
      }

      loopCounter = 0;
    }
  }

};

static SysModSystem *sys;