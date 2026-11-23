#include "steamwood_valve.h"

void func_8017ECE8(struct ValveTimerActor *timer)
{
    if (!func_8016F1C4()) {
        if (--timer->ticks != -1) {
            func_800242D0((short)((timer->ticks + 29) / 30), D_801A0048, 2);
        } else {
            func_800291A0(206, 1);
            func_8017EE30();
            func_8017BEE8();
            func_8016F0AC();
            func_8017DEB0();
        }
    }
}
