#include "tomb_opening.h"

/* Slide the slab, play its movement sound, and set the open flag on the last tick. */
void func_8017E524(CemeteryActor *tomb)
{
    CemeteryActor *effect;
    u32 variation;

    if (++tomb->sound_ticks >= 16) {
        func_8002D4C8(1321, 0);
        tomb->sound_ticks = 0;
    }
    if (--tomb->remaining == 0) {
        tomb->phase = 3;
        func_8017D6E8();
        func_80029124(143, 1);
    }
    tomb->z += 2;
    tomb->x ^= 2;
    effect = func_80143B6C(tomb, 1);
    if (effect) {
        effect->x -= 32;
        variation = func_8005C49C();
        effect->x += variation & 63;
        variation = func_8005C49C();
        effect->z -= variation & 31;
    }
}
