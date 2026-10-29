/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct { u16 x; u16 y; u16 z; u16 w; } V8;
typedef struct { s16 x; s16 y; s16 z; s16 w; } SV;
typedef struct {
    u16 id;
    u8 pad2[0x1E];
    s32 f20;
    u8 pad24[0x34];
    s32 f58;
    u16 f5C;
    u8 pad5E[0x4C];
    s16 fAA;
    u8 padAC[0x10C - 0xAC];
} Slot; /* 0x10C */
typedef struct {
    u8 pad0[6];
    u16 x6;
    u8 pad8[2];
    u16 yA;
    u8 padC[2];
    u16 zE;
    u8 pad10[0x10];
    u8 *p20;
    u8 pad24[0x20];
    u32 f44;
    u8 pad48[0x64];
    u16 fAC;
    u8 padAE[0xCA];
    Slot *f178;
    u8 pad17C[8];
    Slot *f184;
} Obj;

extern u8 D_801202A0[];
extern s32 func_8014BEF8(Obj *o);
extern void func_8012F14C(u8 *m, SV *in, V8 *out);
extern s32 func_80135260(s32 a, s32 b, V8 *p, V8 *q);
extern void func_8014A1B0(Obj *o, Slot *p);

s32 func_8014A048(Obj *o) {
    V8 a;
    V8 b;
    SV c;
    u32 i;
    Slot *p;

    if (o->f44 & 0x400) {
        return 0;
    }
    if (!(o->fAC & 0x80)) {
        if (!(o->fAC & 0x10)) {
            return 0;
        }
        if (func_8014BEF8(o) == 0) {
            goto fail;
        }
    }
    a.x = o->x6;
    a.y = o->yA;
    a.z = o->zE;
    c.y = -0x10;
    c.x = 0;
    c.z = -0x20;
    func_8012F14C(o->p20 + 0x34, &c, &b);
    for (i = 0; i < 0x60; i++) {
        p = &((Slot *)&D_801202A0)[i];
        if (p->id != 0 && p->f58 != 0 && p->fAA == 0 && o->f184 != p && (p->f5C & 0x200) &&
            func_80135260(p->f20, p->f58, &a, &b) != 0) {
            goto found;
        }
    }
    goto fail;
found:
    o->f178 = p;
    func_8014A1B0(o, p);
    return 1;
fail:
    return 0;
}
