#include "steamwood_timer.h"

void func_8017EB58(struct ValveTimerActor *timer)
{
    unsigned int item;

    func_8017EDD8();
    item = func_8012C194();
    if (item) {
        timer->label = item;
        func_80024054(D_80183CD0, D_801A0020);
        func_8001CE28(item, D_801A0020);
        VALVE_HUD_ITEM(item)->x = -D_800AF7BC.w / 2 + 16;
        VALVE_HUD_ITEM(item)->y = -D_800AF7BC.h / 2 + 24;
        VALVE_HUD_ITEM(item)->enabled = 1;
        VALVE_HUD_ITEM(item)->style = 1;
        item = func_8012C194();
        if (item) {
            timer->digits = item;
            func_800242D0((short)(timer->ticks / 30), D_801A0048, 2);
            func_8001CE68(item, D_801A0048);
            VALVE_HUD_ITEM(item)->x = -D_800AF7BC.w / 2 + 24;
            VALVE_HUD_ITEM(item)->y = -D_800AF7BC.h / 2 + 32;
            VALVE_HUD_ITEM(item)->enabled = 1;
            ++timer->phase;
        }
    }
}
