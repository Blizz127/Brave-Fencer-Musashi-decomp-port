/* func_80166438 [80166438,801665B4) - owner re-derivation from retail assembly (clean input set).
 * Spawns the child record (kind 0x9B/0x9C by o->f30 bit 28), copies flags/colours from o and
 * links its source; on failure hands o to func_80146C3C.
 * Reusing the s16 t for the f2 increment (two sets: no sched1 birthing boost) lets sched1 put
 * the a0 arg copy above the tail loads, so optimize_reg_copy_1 bases them on a0 like retail.
 * Callers read v0: none (void). */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    u32 w4;
    u8 pad8[0x10];
    s16 h18;
    s16 h1A;
    u8 pad1C[8];
    u8 b24, b25, b26;
} R;
typedef struct {
    u8 pad0[2];
    u16 f2;
    u8 pad4[0x12];
    s16 h16;
    u8 pad18[8];
    R *f20;
    u8 pad24[8];
    union { u32 w; struct { u16 lo, hi; } h; } f2C;
    s32 f30;
} Obj;

extern u8 D_800D387C[];
extern u8 D_800D3888[];
extern R *func_800D21C4(Obj *o, u8 *a, s32 n);
extern void func_800D1FC8(Obj *o, s32 v);
extern void func_80128EA8(R *r, void *d, u8 *s);
extern void func_80146C3C(Obj *o);
extern void func_801665B4(Obj *o);

void func_80166438(Obj *o) {
    R *r;
    s32 n;
    s32 f;
    s16 t;
    u32 w;

    n = 0x9B;
    if (!(o->f30 & 0x10000000)) {
        n = 0x9C;
    }
    r = func_800D21C4(o, D_800D387C, n);
    if (r != 0) {
        o->f20 = r;
        t = o->f30 & 0x7FFF;
        r->h1A = t;
        r->h18 = t;
        if (o->f30 & 0x8000) {
            func_800D1FC8(o, 6);
        }
        f = o->f30;
        if (f < 0) {
            r->w4 = r->w4 | 0x50000000;
        } else if (f & 0x08000000) {
            r->w4 = 0x60000000;
        }
        if (!(o->f30 & 0x40000000)) {
            r->w4 = r->w4 | 0x40;
        } else {
            r->w4 = r->w4 & ~0x40;
        }
        if (o->f30 & 0x20000000) {
            r->b24 = (o->f2C.w >> 20) & 0xF0;
            w = o->f2C.h.hi;
            r->b25 = w & 0xF0;
            r->b26 = (o->f2C.w >> 12) & 0xF0;
        }
        func_80128EA8(r, (u8 *)o + 0x24, D_800D3888);
        o->h16 = o->f2C.w;
        t = o->f2 + 1;
        o->f2 = t;
        func_801665B4(o);
        return;
    }
    func_80146C3C(o);
}
