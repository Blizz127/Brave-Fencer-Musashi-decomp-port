/* func_801298F4 [801298F4,801299C8) - owner re-derivation from retail assembly (clean input set). */
/* Initialises a sprite/draw record from its source headers at +0x4C/+0x50 (four header bytes,
 * texture flags from byte 6, copies the +0x38 rect) and sets default colour/scale fields. */
#include "psx_types.h"

/* o->f4C / o->f50 are guest address words (u32); o->f24 gets the object's own address. */
typedef struct {
    u8 f0;
    u8 f1;
    u16 f2;
    u16 f4;
    u8 p6[2];
    u32 f8;
    u32 fC;
    u32 f10;
    u16 f14;
    u16 f16;
    u16 f18;
    u16 f1A;
    u16 f1C;
    u16 f1E;
    u8 f20;
    u8 f21;
    u8 f22;
    u8 p23;
    u32 f24;
    u16 f28;
    u16 f2A;
    u16 f2C;
    u16 f2E;
    u32 f30;
    u8 p34[4];
    u16 f38;
    u16 f3A;
    u16 f3C;
    u16 f3E;
    u8 p40[0xC];
    u32 f4C;
    u32 f50;
} O;

/* callers read v0: no (the only direct caller, 80129504, ignores it; retail sets none); void. */
extern u16 D_800B99F0;
/* in-struct view of D_800B99F0 at its one use: the load then stays after the struct stores */
typedef struct {
    u16 v;
} Rec801298F4;

void func_801298F4(O *o)
{
    u8 *p;
    u32 t;
    u8 *q;
    u32 u;

    q = (u8 *)o->f4C;
    q += 8;
    p = (u8 *)o->f50;
    p += 4;
    t = *p++;
    o->f2 = t;
    t = *p++;
    o->f4 = t;
    t = *p++;
    o->f0 = t;
    t = *p++;
    o->f8 = (u32)q;
    o->fC = (u32)p;
    o->f1 = t;
    u = q[6];
    o->f14 = o->f38;
    o->f16 = o->f3A;
    o->f18 = o->f3C;
    o->f1A = o->f3E;
    o->f10 = ((u >> 7) << 24) | 0x8000000;
    if (((Rec801298F4 *)&D_800B99F0)->v == 0) {
        o->f1E = 0;
        o->f1C = 0;
    }
    o->f22 = 0x80;
    o->f21 = 0x80;
    o->f20 = 0x80;
    o->f28 = 0xA0;
    o->f2A = 0x78;
    o->f24 = (u32)o;
    o->f2E = 0x1000;
    o->f2C = 0x1000;
    o->f30 = 0;
}
