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
#pragma once

#include <Arduino.h>
/*
#ifdef DO_DEBUG
#define LOG_DEBUG(...) log::vlogf(LOG_DEBUG, __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

#ifdef DO_INFO
#define LOG_INFO(...) log::vlogf(LOG_INFO, __VA_ARGS__)
#else
#define LOG_INFO(...)
#endif
*/

enum loglevel { LOG_DEBUG =1,
                LOG_INFO,
                LOG_WARN,
                LOG_ERROR,
                LOG_FATAL };

class esplog
{
    bool logSerial;
    loglevel logLevel;

  public:
    void setup(bool doSerial, loglevel doLevel);
    void setLoglevel(loglevel doLevel);
    void logMessage(uint8_t level, char *message);
    void vlogf(uint8_t level, const char *fmt, ...);
    void log(uint8_t level, const char *message);
};

extern esplog logger;
