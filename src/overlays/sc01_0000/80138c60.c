/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the sc01_0005 twin at this address (W18). */
/* Returns 1 at once when bit 13 of the flags word is set. Otherwise stores the low three bits of the
 * byte at 0x22 as the u16 at 0x18 (clearing the byte at 0x20 for values below 2), merges the flag
 * word's byte into the byte at 0x22 when bit 6 of the flags is set, picks a 4-byte entry of the
 * direction table by the direction bits of that byte (bits 3-4 when bit 7 is set, else bits 3-6, also
 * clearing the u16 at 0x1C when bits 5-6 are set), copies it unaligned to offset 0x24, then runs the
 * three follow-up steps on the object and returns the last one's result. */
#include "psx_types.h"

typedef struct { u8 b[4]; } Raw4;

typedef struct {
    u8 pad0[8];
    u32 flags;     /* 0x08 */
    u8 pad0c[0xC];
    u16 h18;       /* 0x18 */
    u8 pad1a[2];
    u16 h1C;       /* 0x1C */
    u8 pad1e[2];
    u8 b20;        /* 0x20 */
    u8 pad21;
    u8 bits;       /* 0x22 */
    u8 pad23;
    Raw4 dir;      /* 0x24 */
} Obj;

extern Raw4 D_8017F1D8[];
extern void func_80139BE0(Obj *o);
extern void func_80139C7C(Obj *o);
extern s32 func_8013A8FC(Obj *o);

s32 func_80138C60(Obj *o)
{
    s32 f;
    u32 b;
    s32 idx;
    u8 k;

    if (o->flags & 0x2000) {
        return 1;
    }
    k = o->bits & 7;
    o->h18 = k;
    if (o->h18 < 2) {
        o->b20 = 0;
    }
    f = o->flags;
    if (f & 0x40) {
        o->bits = (o->bits & 0x67) | (f & -0x68);
    }
    b = o->bits;
    if (b & 0x80) {
        idx = (b & 0x18) >> 3;
    } else {
        idx = (b & 0x78) >> 3;
        if (b & 0x60) {
            o->h1C = 0;
        }
    }
    o->dir = D_8017F1D8[idx];
    func_80139BE0(o);
    func_80139C7C(o);
    return func_8013A8FC(o);
}
