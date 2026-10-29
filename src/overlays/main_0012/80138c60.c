/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} Rgba80138C60; /* alignment 1: the copy uses lwl/lwr/swl/swr */

typedef struct {
    u8 pad00[8];
    u32 f08;
    u8 pad0C[0x0C];
    u16 f18;
    u8 pad1A[2];
    u16 f1C;
    u8 pad1E[2];
    u8 f20;
    u8 pad21;
    u8 f22;
    u8 pad23;
    Rgba80138C60 color;
} Obj80138C60;

extern Rgba80138C60 D_8017E950[];
extern void func_80139BE0(Obj80138C60 *o);
extern void func_80139C7C(Obj80138C60 *o);
extern s32 func_8013A8FC(Obj80138C60 *o);

s32 func_80138C60(Obj80138C60 *o) {
    u16 mh;
    s32 idx;
    s32 fl;
    u8 v;

    if (o->f08 & 0x2000) {
        return 1;
    }
    mh = (u8)(o->f22 & 7);
    o->f18 = mh;
    if (mh < 2) {
        o->f20 = 0;
    }
    fl = o->f08;
    if (fl & 0x40) {
        o->f22 = (o->f22 & 0x67) | (fl & ~0x67);
    }
    v = o->f22;
    if (v & 0x80) {
        idx = (v & 0x18) >> 3;
    } else {
        idx = (v & 0x78) >> 3;
        if (v & 0x60) {
            o->f1C = 0;
        }
    }
    o->color = D_8017E950[idx];
    func_80139BE0(o);
    func_80139C7C(o);
    return func_8013A8FC(o);
}
