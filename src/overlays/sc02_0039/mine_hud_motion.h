#ifndef BFM_MINE_HUD_MOTION_H
#define BFM_MINE_HUD_MOTION_H

#include "mine_frame_slots.h"

/* Byte 2 holds the counter, byte 3 the initial value and bytes 4-5 the
 * sliding X coordinate. The four original slots start on word boundaries. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define mine_hud_x(slot) (*(u16 *)((slot)->reserved + 2))
#define mine_hud_set_x(slot, value) (mine_hud_x(slot) = (value))
#else
static __inline__ u16 mine_hud_x(const MineFrameSlot *slot)
{
    return (u16)(slot->reserved[2] | ((u16)slot->reserved[3] << 8));
}

static __inline__ void mine_hud_set_x(MineFrameSlot *slot, u16 x)
{
    slot->reserved[2] = (u8)x;
    slot->reserved[3] = (u8)(x >> 8);
}
#endif

#endif
