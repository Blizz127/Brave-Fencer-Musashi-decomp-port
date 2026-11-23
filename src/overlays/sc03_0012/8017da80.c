#include "vent_phase.h"

void func_8017DA80(SteamwoodVentPhase *actor)
{
    if (--actor->phase_delay == 0) {
        func_8017D780();
        actor->activated = 1;
        func_8002D4C8(0x555, 0);
        ++actor->phase;
    }

    if (--actor->sequence_delay == 0) {
        actor->sequence_delay = (D_800B99DC & 7) + 1;
        func_8013C9C4(D_8017E8A4);
    }
}
