#include "steamwood_timer.h"

void func_8017EE30(void)
{
    struct ValveTimerActor *timer = func_8012E544(199);
    if (timer) {
        if (timer->digits)
            func_80016714(timer->digits, 56);
        if (timer->label)
            func_80016714(timer->label, 56);
        timer->phase = 0;
    }
}
