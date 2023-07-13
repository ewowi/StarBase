#include "SysModModel.h"
#include "module.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModFiles.h"

bool SysModModel::doWriteModel = false;
bool SysModModel::doShowObsolete = false;
DynamicJsonDocument * SysModModel::model = nullptr;

  void SysModModel::setup() {
    Module::setup();

    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);

    ui->initDisplay(parentObject, "mSize", nullptr, [](JsonObject object) {
      web->addResponse(object, "label", "Size");
    });

    ui->initButton(parentObject, "saveModel", "SaveModel", nullptr, [](JsonObject object) {
      doWriteModel = true;
    });

    ui->initCheckBox(parentObject, "showObsolete", false, [](JsonObject object) {
      web->addResponse(object, "comment", "Show in UI (refresh)");
    }, [](JsonObject object) {
      doShowObsolete = object["value"];
    });

    ui->initButton(parentObject, "deleteObsolete", "DeleteObsolete", [](JsonObject object) {
      web->addResponse(object, "label", "Delete obsolete objects");
    }, [](JsonObject object) {
    });

    ui->initButton(parentObject, "deleteModel", "DeleteModel", nullptr, [](JsonObject object) {
      print->print("delete model json\n");
      files->remove("/model.json");
    });

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  bool SysModModel::readObjectFromFile(const char* path, JsonDocument* dest) {
    // if (doCloseFile) closeFile();
    File f = files->open(path, "r");
    if (!f) {
      print->print("File %s open not successful %s\n", path);
      return false;
    }
    else { 
      print->print(PSTR("File %s open to read, size %d bytes\n"), path, (int)f.size());
      DeserializationError error = deserializeJson(*dest, f);
      if (error) {
        print->print("readObjectFromFile deserializeJson failed with code %s\n", error.c_str());
        f.close();
        return false;
      } else {
        f.close();
        return true;
      }
    }
  }

  bool SysModModel::writeObjectToFile(const char* path, JsonDocument* dest) {
    File f = files->open(path, "w");
    if (f) {
      print->println(F("  success"));
      serializeJson(*dest, f);
      return true;
    } else {
      f.close();
      print->println(F("  fail"));
      return false;
    }
  }
