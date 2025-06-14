#include "IBT4.h"

IBT4::IBT4()
{
    active = false;
};

IBT4::IBT4(uint8_t pinIn1, uint8_t pinIn2) : in1(pinIn1), in2(pinIn2)
{
    active = false;
    frequency = 200; // default 200Hz wit 8 bit resolution; 200 is the minimum frequency
    resolution = 8;
    channel = 1;
    duty = 255;
}

IBT4::IBT4(uint8_t pinIn1, uint8_t pinIn2, uint32_t freq, uint8_t res, uint8_t chan) : in1(pinIn1), in2(pinIn2), frequency(freq), resolution(res), channel(chan)
{
    active = false;
}

void IBT4::begin()
{
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    stop();
    ledcSetup(channel, frequency, resolution);
}
void IBT4::setIn1High()
{
    ledcDetachPin(in2);
    digitalWrite(in2, LOW);
    ledcAttachPin(in1, channel);
    ledcWrite(channel, duty);
}
void IBT4::setIn1High(uint32_t dt)
{
    duty = dt;
    setIn1High();
}
void IBT4::setIn2High()
{
    ledcDetachPin(in1);
    digitalWrite(in1, LOW);
    ledcAttachPin(in2, channel);
    ledcWrite(channel, duty);
}
void IBT4::setIn2High(uint32_t dt)
{
    duty = dt;
    setIn2High();
}

uint32_t IBT4::getDuty()
{
    return duty;
}
void IBT4::stop()
{
    ledcDetachPin(in1);
    digitalWrite(in1, LOW);
    ledcDetachPin(in2);
    digitalWrite(in2, LOW);
    active = false;
}
