#include "configfile.h"
#include <SPIFFS.h>

ConfigFile::ConfigFile()
{
    // set defaults

    iniLoglevel = LOG_INFO; // INFO
    iniFastBoot = false;    // do not skip the setup messages

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
            iniLoglevel = (loglevel) value.toInt();
        }
        if (key == "FASTBOOT") {
            iniFastBoot = value.toInt() == 1;
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