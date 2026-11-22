/* The signed input arms this slot. The first update reads its stored low byte. */
#include "mine_frame_slots.h"

void func_8016E918(s16 index, s16 value)
{
    if (value < 10) {
        MineFrameSlot *slot = &D_80115218[index];
        slot->active = index + 1;
        slot->reserved[1] = value;
        slot->handler = 0;
    }
}
