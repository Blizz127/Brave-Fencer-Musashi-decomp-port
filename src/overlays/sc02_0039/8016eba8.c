/* The last draw is submitted before this slot becomes inactive. */
#include "mine_hud_motion.h"

void func_8016EBA8(MineFrameSlot *slot)
{
    u8 prior;

    mine_hud_set_x(slot, mine_hud_x(slot) - D_801824BC[slot->reserved[0]]);
    func_8016EC0C(slot, 128);
    prior = slot->reserved[0];
    slot->reserved[0] = prior + 255;
    if (prior == 0) slot->active = 0;
}
