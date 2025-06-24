#include "configfile.h"
#include <SPIFFS.h>

ConfigFile::ConfigFile()
{
    // set defaults

    iniLoglevel = LOG_INFO; // INFO

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