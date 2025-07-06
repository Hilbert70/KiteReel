#pragma once

class motordirection
{
  public:
    motordirection();
    void setValue(long newValue, bool newImmediateStop = false);
    long getValue(bool resetChange = true);
    bool isChanged();
    bool isImmediateStop(bool resetImmediateStop = true);

  protected:
    long value;
    bool changed;
    bool immediateStop;
};