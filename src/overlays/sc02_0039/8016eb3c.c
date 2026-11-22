/* The hold phase selects the current tint before advancing its countdown. */
#include "mine_frame_slots.h"

void func_8016EB3C(MineFrameSlot *slot)
{
    u8 prior;

    func_8016EC0C(slot, D_801824AC[slot->reserved[0] & 3]);
    prior = slot->reserved[0];
    slot->reserved[0] = prior + 255;
    if (prior != 0) return;
    slot->reserved[0] = 5;
    ++slot->handler;
}
