#include "steamwood_valve.h"

void func_8017EDD8(void)
{
    unsigned char progress = func_800291B4(206);
    struct ValveTimerActor *timer = func_8012E544(199);
    if (timer)
        timer->ticks = D_80183CF4[progress];
}
