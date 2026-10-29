/* Owner re-derivation from retail assembly (clean input set). */
/* Initialises the record at +0x20: callback pointer D_800D387C, default flags word 0x50000000, byte 0x9C at +0x27, the 15-bit value of the halfword at +0x34 copied into +0x18/+0x1A when nonzero; hands the record, o+0x24 and the table word selected by the low two bits to func_80128EA8 and advances the state halfword. */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    u32 w4;
    u8 pad8[0x10];
    s16 h18;
    s16 h1A;
    u8 pad1C[4];
    u8 *p20;
    u8 pad24[3];
    u8 b27;
} Rec;

typedef struct {
    u8 pad0[2];
    u16 sel;
    u8 pad4[0x1C];
    Rec *rec;
    u8 pad24[0x10];
    u16 h34;
} Obj;

extern u8 D_800D387C[];
extern u32 D_80185CCC[];
extern void func_80128EA8(Rec *r, void *p, u32 w);

void func_80143458(Obj *o)
{
    s32 t;

    o->rec->p20 = D_800D387C;
    if (o->rec->w4 == 0) {
        o->rec->w4 = 0x50000000;
    }
    o->rec->b27 = 0x9C;
    t = o->h34 & 0x7FFF;
    if (t != 0) {
        o->rec->h18 = o->rec->h1A = t;
    }
    func_80128EA8(o->rec, (u8 *)o + 0x24, D_80185CCC[o->h34 & 3]);
    o->sel = o->sel + 1;
}
