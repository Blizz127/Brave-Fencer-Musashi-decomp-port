/* Active records dispatch in ascending order, with the handler read on each visit. */
#include "mine_frame_slots.h"

void func_8016E95C(void)
{
    MineFrameSlot *slot;

    for (slot = D_80115218; slot < &D_80115218[4]; ++slot) {
        if (slot->active != 0) {
            D_801824C8[slot->handler](slot);
        }
    }
}
