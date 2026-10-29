/* Owner re-derivation from retail assembly (clean input set). */
/* Unpacks three signed 10-bit fields (scaled by 8) from the word at +0x2C of the object (bits 29-20 = x, 19-10 = z, 9-0 = y) and stores two angles: +0x12 = -angle(z, hypot(x, y)), +0x16 = angle(x, y); +0x1A = 0. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x12];
    s16 a12;
    u8 pad14[2];
    s16 a16;
    u8 pad18[2];
    s16 a1A;
    u8 pad1C[0x10];
    u32 packed;
} Obj800D2318;

extern s32 func_80047D3C(s32 v);
extern s32 func_8004CFEC(s32 y, s32 x);

void func_800D2318(Obj800D2318 *o)
{
    u32 w;
    u32 m;
    s32 x;
    s32 y;
    s32 z;
    s32 r;

    w = o->packed;
    m = w & 0x3FFFFFFF;
    x = (s16)((m >> 14) & 0xFFC0) >> 3;
    y = (s32)(w << 22) >> 19;
    z = (s16)((m >> 4) & 0xFFC0) >> 3;
    r = func_80047D3C(x * x + y * y);
    o->a12 = -func_8004CFEC(z, (s16)r);
    o->a16 = func_8004CFEC(x, y);
    o->a1A = 0;
}
