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
    bool getBluetooth();
    String getBLEName();
    uint32_t getBLEPIN();
    String getWifiSSID();
    String getWifiPassword();
    String getOTAPassword();
    String getHostname();

  protected:
    loglevel iniLoglevel;
    bool iniFastBoot;
    float iniMaxCurrent;
    float iniMinVoltage;
    bool iniBluetooth;
    String iniBLEName;
    uint32_t iniBLEPIN;
    String iniWifiSSID;
    String iniWifiPassword;
    String iniOTAPassword;
    String iniHostname;
};