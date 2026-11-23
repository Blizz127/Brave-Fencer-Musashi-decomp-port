#include "steamwood_timer.h"

void func_8017EB14(struct ValveTimerActor *timer)
{
    D_80183CE8[timer->phase](timer);
}
