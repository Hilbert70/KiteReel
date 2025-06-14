#pragma once

#include <Arduino.h>

class IBT4
{
  public:
    IBT4();
    IBT4(uint8_t pinIn1, uint8_t pinIn2);
    IBT4(uint8_t pinIn1, uint8_t pinIn2, uint32_t freq, uint8_t res, uint8_t chan);

    void begin();

    void setIn1High();
    void setIn1High(uint32_t duty);
    void setIn2High();
    void setIn2High(uint32_t duty);

    uint32_t getDuty();
    void stop();

  protected:
    uint8_t in1;
    uint8_t in2;
    uint32_t duty;
    uint32_t frequency;
    uint8_t resolution;
    uint8_t channel;

    bool in1Active;
    bool active;
};