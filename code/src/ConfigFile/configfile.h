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
    float getMaxCurrent();
    float getMinVoltage();

  protected:
    loglevel iniLoglevel;
    bool iniFastBoot;
    float iniMaxCurrent;
    float iniMinVoltage;
};