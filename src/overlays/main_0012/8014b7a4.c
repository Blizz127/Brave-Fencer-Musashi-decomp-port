/* func_8014B7A4 [8014B7A4,8014B944) - owner re-derivation from retail assembly (clean input set).
 * Per-frame timer step: picks the effect by o->f44 / area id, then advances the area timer
 * (0xAAA8 fast / 0x1555) and raises o->f44 bit 8 at the limit. Both selects are `X op= K` in
 * each arm (cross-jumped store); a shared result local swaps the constant/value registers.
 * Callers read v0: none (void). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x2C];
    s32 f2C;
    union { s32 w; struct { u16 lo; s16 hi; } h; } f30;
} G;
typedef struct {
    u8 pad0[0xA3D8];
    s16 fA3D8;
} H;
typedef struct {
    u16 id;
    u8 pad2[0x42];
    u32 f44;
} Obj;

extern u8 D_80078E78[];
extern u8 D_800AF630[];
extern s16 D_800B9A08;
extern u32 D_80078EA4;
extern s32 func_8016F1C4(void);
extern u8 func_80029178(s32 a);
extern void func_8014B944(Obj *o, s32 a, s32 b);

void func_8014B7A4(Obj *o) {
    G *g;
    H *h;
    u16 t;
    u32 f;

    h = ((H *)&D_800AF630);
    g = ((G *)&D_80078E78);
    if (func_8016F1C4() == 0) {
        t = o->id;
        if (t != 0x1E && t != 0x1A) {
            f = o->f44;
            if (!(f & 0x10)) {
                if (f & 0x80) {
                    if (func_80029178(0x1B)) {
                        func_8014B944(o, 0x2D80, 0);
                    } else {
                        func_8014B944(o, 0x16C0, 0);
                    }
                } else if (D_800B9A08 != 0x3024 && D_800B9A08 != 0x3042 && D_800B9A08 != 0x3067 && D_800B9A08 != 0x3075) {
                    if (func_80029178(0x21)) {
                        D_80078EA4 -= 0xF2;
                    } else {
                        D_80078EA4 -= 0x16C;
                    }
                }
                if (g->f2C < 0) {
                    g->f2C = 0;
                    if (o->f44 & 0x80) {
                        g->f30.w += 0xAAA8;
                    } else {
                        g->f30.w += 0x1555;
                    }
                    if (g->f30.h.hi >= 0x3C) {
                        g->f30.w = 0;
                        if (h->fA3D8 != 0x3051) {
                            o->f44 = o->f44 | 0x100;
                        }
                    }
                }
            }
        }
    }
}


