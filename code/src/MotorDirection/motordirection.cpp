#include "motordirection.h"

motordirection::motordirection()
{
    changed = false;
}
void motordirection::setValue(long newValue,bool newImmediateStop)
{
    changed = true;
    immediateStop = newImmediateStop;
    value = newValue;
}
long motordirection::getValue(bool resetChange)
{
    if (resetChange)
        changed = false;
    return value;
}

bool motordirection::isChanged()
{
    return changed;
}

bool motordirection::isImmediateStop(bool resetImmediateStop)
{
    bool oldImmediateStop = immediateStop;
    if (resetImmediateStop) {
        immediateStop = false;
    }
    return oldImmediateStop;
}
