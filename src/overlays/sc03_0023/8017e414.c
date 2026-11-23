#include "tomb_opening.h"

/* Process the selected slab and start the linked actors' opening phases. */
void func_8017E414(CemeteryActor *tomb)
{
    CemeteryPlayer *player = &D_80126B58;

    if (!D_801270C8) {
        tomb->interaction &= ~0x1000;
        return;
    }
    if (D_80126B78->heading == 0x800)
        tomb->interaction |= 0x1000;
    else
        tomb->interaction &= ~0x1000;

    if (tomb == player->selected) {
        volatile CemeteryActor *slab = tomb;
        volatile CemeteryActor *linked;

        tomb->interaction &= ~0x1000;
        func_8017D6C0();
        slab->phase = 2;
        slab->remaining = 40;
        linked = D_801270C8;
        linked->phase = 4;
        linked = D_801270C8;
        linked->remaining = 20;
        func_8012A828(D_801270C8, D_801932E8);
        func_8012C658(136, 0, tomb);
        func_8002D4C8(1321, 0);
        tomb->sound_ticks = 0;
    }
}
