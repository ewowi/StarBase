#include "module.h"
#include "SysModPrint.h"
#include "SysModUI.h"

  void SysModPrint::setup() {
    Module::setup();

    print("%s %s\n", __PRETTY_FUNCTION__, name);

    parentObject = ui->initGroup(parentObject, name);

    ui->initDisplay(parentObject, "log");

    print("%s %s %s\n",__PRETTY_FUNCTION__,name, success?"success":"failed");
  }

  size_t SysModPrint::print(const char * format, ...) {
    va_list args;
    va_start(args, format);

    size_t len = vprintf(format, args);

    va_end(args);
    
    // ui->setValueV("log", "%lu", millis());

    return len;
  }

