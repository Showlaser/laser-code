#include "Watchdog_t4.h"
#include "OledModule.h"

class IMode
{
public:
    virtual void init(WDT_T4<WDT1> &watchdog, OledModule oled) = 0;
    virtual void execute() = 0;
    virtual void forceStop();
    virtual ~IMode() {}
};
