#pragma once

#include "Esplog/esplog.h"
#include <Arduino.h>

class ConfigFile
{
  public:
    ConfigFile();
    ~ConfigFile();

    loglevel getLoglevel();
    bool getFastBoot();

  protected:
    loglevel iniLoglevel;
    bool iniFastBoot;
};