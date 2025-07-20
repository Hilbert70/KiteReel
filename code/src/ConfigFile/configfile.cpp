#include "configfile.h"
#include <SPIFFS.h>

ConfigFile::ConfigFile()
{
    // set defaults

    iniLoglevel = LOG_INFO; // INFO
    iniFastBoot = false;    // do not skip the setup messages
    iniMaxCurrent = 1.0;
    iniMinVoltage = 3.6;
    iniBluetooth = false;
    iniBLEName = "KiteReel";
    iniBLEPIN = 1234;
    iniWifiSSID = "";
    iniWifiPassword = "";
    iniOTAPassword = "";
    iniHostname = "";

    if (!SPIFFS.begin(true)) {
        logger.log(LOG_FATAL, "An error has occurred while mounting SPIFFS");
        return;
    }

    File file = SPIFFS.open("/config.ini", "r");
    if (!file) {
        logger.log(LOG_WARN, "Failed to open file for reading");
        return;
    }
    while (file.available()) {
        String line = file.readStringUntil('\n');
        int separatorIndex = line.indexOf('=');
        if (separatorIndex == -1)
            continue;

        String key = line.substring(0, separatorIndex);
        String value = line.substring(separatorIndex + 1);

        if (key == "LOGLEVEL") {
            iniLoglevel = (loglevel)value.toInt();
        }
        if (key == "FASTBOOT") {
            iniFastBoot = value.toInt() == 1;
        }
        if (key == "MAXCURRENT") {
            iniMaxCurrent = value.toFloat();
        }
        if (key == "MINVOLTAGE") {
            iniMinVoltage = value.toFloat();
        }
        if (key == "BLUETOOTH") {
            iniBluetooth = value.toInt() == 1;
        }
        if (key == "BLENAME") {
            iniBLEName = value;
        }
        if (key == "BLEPIN") {
            iniBLEPIN = value.toInt();
        }
        if (key == "WIFISSID") {
            iniWifiSSID = value;
        }
        if (key == "WIFIPASSWORD") {
            iniWifiPassword = value;
        }
        if (key == "OTAPASSWORD") {
            iniOTAPassword = value;
        }
        if (key == "HOSTNAME") {
            iniHostname = value;
        }
    }
    file.close();
}
ConfigFile::~ConfigFile()
{
    // nothing yet
}

loglevel ConfigFile::getLoglevel()
{
    return iniLoglevel;
}

bool ConfigFile::getFastBoot()
{
    return iniFastBoot;
}

float ConfigFile::getMaxCurrent()
{
    return iniMaxCurrent;
}

float ConfigFile::getMinVoltage()
{
    return iniMinVoltage;
}

bool ConfigFile::getBluetooth()
{
    return iniBluetooth;
}

String ConfigFile::getBLEName()
{
    return iniBLEName;
}

uint32_t ConfigFile::getBLEPIN()
{
    return iniBLEPIN;
}

String ConfigFile::getWifiSSID()
{
    return iniWifiSSID;
}

String ConfigFile::getWifiPassword()
{
    return iniWifiPassword;
}

String ConfigFile::getOTAPassword()
{
    return iniOTAPassword;
}
String ConfigFile::getHostname()
{
    return iniHostname;
}