/* Slide in, draw, then advance the counter visible after the callback. */
#include "mine_hud_motion.h"

void func_8016EAC0(MineFrameSlot *slot)
{
    mine_hud_set_x(slot, mine_hud_x(slot) + D_801824BC[slot->reserved[0]]);
    func_8016EC0C(slot, 128);
    ++slot->reserved[0];
    if (slot->reserved[0] >= 6) {
        slot->reserved[0] = 32;
        ++slot->handler;
    }
}
