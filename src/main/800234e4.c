/* Owner re-derivation from retail assembly (clean input set). */
/* Fills 12 (x, y) halfword pairs at +0x10 with points on a circle of the given radius, starting at the given angle. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
} Point;

typedef struct {
    u8 pad0[0x10];
    Point pt[12];
} Ring;

extern s32 func_8004787C(s32);
extern s32 func_80047948(s32);

void func_800234E4(Ring *ring, s32 radius, s32 angle)
{
    s32 i;
    s32 r;

    i = 0;
    r = radius & 0xFFFF;
    angle &= 0xFFFF;
    do {
        ring->pt[i].x = (func_8004787C(angle + i * 0x155) * r) >> 12;
        ring->pt[i].y = (func_80047948(angle + i * 0x155) * r) >> 12;
        i++;
    } while (i < 12);
}
