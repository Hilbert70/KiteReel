#pragma once

#include <Arduino.h>
#include "Esplog/esplog.h"

class ConfigFile
{
  public:
    ConfigFile();
    ~ConfigFile();

    loglevel getLoglevel();
  protected:
    loglevel iniLoglevel;
};