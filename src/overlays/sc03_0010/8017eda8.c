#include "steamwood_valve.h"

void func_8017EDA8(void)
{
    struct ValveTimerActor *timer = func_8012E544(199);
    if (timer)
        timer->phase = 1;
}
