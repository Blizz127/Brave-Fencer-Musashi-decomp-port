/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up a byte row by o->f2C (D_80187C10) into o->f30, then a second pair of byte
 * tables (D_80187BE8/E9) by (s16)(o->f2C << 1) into o->f2A/f1C, triggers func_80166F58(o,
 * 5, 0x3B, 2), and bumps the o->f2 frame counter. void, no caller found in any region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2;      /* 0x2 */
    u8 pad4[0x1C - 0x4];
    u32 f1C;     /* 0x1C */
    u8 pad20[0x2A - 0x20];
    u16 f2A;     /* 0x2A */
    s32 f2C;     /* 0x2C */
    u32 f30;     /* 0x30 */
} O;

extern u8 D_80187C10[];
extern u8 D_80187BE8[];
extern u8 D_80187BE9[];
extern void func_80166F58(O *o, s32 a1, s32 a2, s32 a3);

void func_80168430(O *o)
{
    s32 idx;

    o->f30 = D_80187C10[o->f2C];
    idx = (s16)(o->f2C << 1);
    o->f2A = D_80187BE8[idx];
    o->f1C = D_80187BE9[idx];
    func_80166F58(o, 5, 0x3B, 2);
    o->f2 = o->f2 + 1;
}
