/* Owner re-derivation from retail assembly (clean input set). */
/* Build a twelve-point ring of the given radius: one quadrant from sin/cos, the rest mirrored. */
#include "psx_types.h"

typedef struct Pt {
    s16 x;
    s16 y;
} Pt;

typedef struct Ring {
    u8 pad[0x10];
    Pt pt[12];
} Ring;

extern s32 func_8004787C(s32 a);
extern s32 func_80047948(s32 a);

void func_800233CC(Ring *ring, u16 r)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        ring->pt[i].x = (func_8004787C(i * 0x155) * r) >> 12;
        ring->pt[i].y = (func_80047948(i * 0x155) * r) >> 12;
    }
    for (i = 4; i < 7; i++) {
        ring->pt[i].x = ring->pt[6 - i].x;
        ring->pt[i].y = -ring->pt[6 - i].y;
    }
    for (i = 7; i < 12; i++) {
        ring->pt[i].x = -ring->pt[12 - i].x;
        ring->pt[i].y = ring->pt[12 - i].y;
    }
}
