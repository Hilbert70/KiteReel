/*
MIT License

Copyright (c) 2023 Hilbert Barelds

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "esplog.h"

void esplog::setup(bool doSerial, loglevel doLevel)
{
    logSerial = doSerial;
    logLevel = doLevel;
}

void esplog::logMessage(uint8_t level, char *message)
{
    if (logSerial && (level >= logLevel)) {
        Serial.println(message);
    }
}

void esplog::vlogf(uint8_t level, const char *fmt, ...)
{
    char *message;
    va_list args;

    va_start(args, fmt);

    size_t initialLen;
    size_t len;

    initialLen = strlen(fmt);

    message = new char[initialLen + 11];
    len = vsnprintf(message, initialLen + 1, fmt, args);
    if (len > initialLen) {
        delete[] message;
        message = new char[len + 11];
        vsnprintf(message, len + 1, fmt, args);
    }
    va_end(args);

    log(level, message);
    delete[] message;
}

void esplog::log(uint8_t level, const char *message)
{
    char *msg = NULL;
    unsigned int len = strlen(message);
    msg = new char[len + 20];

#ifdef ESP32
    sprintf(msg, "[%04d] %s", xPortGetCoreID(), message);
    msg[strlen(msg)] = '\0'; // should be paded by zeros, HAVE TO TEST
#else
    sprintf(msg, "%s", message);
    msg[len] = '\0';
#endif

    logMessage(level, msg);
    delete[] msg;
}

void esplog::setLoglevel(loglevel doLevel)
{
    logLevel = doLevel;
}

esplog logger = esplog();