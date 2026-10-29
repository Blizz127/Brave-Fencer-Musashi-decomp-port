/* Owner re-derivation from retail assembly (clean input set). */
/* Probe query: runs the func_80135004 test then scans the slot pool for the first eligible slot hit by the probe; fills the caller's out contact/vector/slot pointers and returns the hit flags (bit 1 direct hit, 2 pre-hit, 4 flagged hit).
 * callers read v0: yes (the wrapper returns it), so s32. a2 and p10 are u16 params that stay spilled (sh at entry,
 * lhu reload): the a2 & 0x8000 test is written inside the loop (loop.c hoists it after the (u16)id extend) and
 * the p10 test as p->sAE & p10 (operand order keeps its zero-extend in the loop). */
#include "psx_types.h"

typedef struct {
    u16 x;
    u16 y;
    u16 z;
    u16 w;
} V8;

typedef struct {
    u16 f0;
    u8 p2[0x1E];
    u32 f20;
    u8 p24[0x34];
    u32 f58;
    union {
        u32 w;
        struct {
            u16 lo;
            u16 hi;
        } h;
    } f5C;
    u8 p60[0x15];
    u8 f75;
    u8 p76[0x38];
    s16 sAE;
    u8 pB0[0x10C - 0xB0];
} Slot;

extern u8 D_801202A0[];
extern s16 D_80126720;
extern u8 D_801152A8[];
extern s32 func_80135004(s32 a, void *p, void *q);
extern s32 func_80135260(s32 a, s32 b, void *p, void *q);
extern s32 func_8014C918(void *o, s32 a);
extern void func_8014C4AC(Slot *p, s32 a, s32 b, V8 *c, s32 d);

s32 func_801571C4(u8 *o, u16 id, u16 a2, void *a3, V8 *p4, V8 *p5, V8 *p6, Slot **p7, V8 *p8, s32 p9, u16 p10, u8 *p11, u8 *p12) {
    s32 r;
    u32 i;
    Slot *p;
    V8 v;
    r = 0;
    if (func_80135004(1, a3, p4) != 0) {
        *p11 = p4->w >> 13;
        *p12 = p4->w;
        *p6 = *(V8 *)D_801152A8;
        r = 2;
    }
    for (i = 0; i < 0x60; i++) {
        p = &((Slot *)D_801202A0)[i];
        if (p->f0 == 0) continue;
        if (p->f58 == 0) continue;
        if ((p->f5C.w & 0x8100) != 0x8000) continue;
        if (p->f5C.h.hi == 0xA) continue;
        if (!(p9 & 0x80) && p->f5C.h.hi == id) continue;
        if (id == 0xD && !(p->f5C.h.lo & 0x2000)) continue;
        if (func_80135260(p->f20, p->f58, a3, p4) == 0) continue;
        if ((p->f5C.h.lo & 0x4000) || (p->sAE & p10)) {
            *p11 = func_8014C918(o, p->f75);
            *p12 = 0;
            *p6 = *(V8 *)D_801152A8;
            r |= 4;
            *p7 = p;
        } else {
            v = *p4;
            if ((a2 & 0x8000) != 0) {
                v.w = 1;
            } else {
                v.w = 0;
            }
            func_8014C4AC(p, id, a2 & 0x7FFF, &v, *(s16 *)(*(u8 **)(o + 0x20) + 0x12));
            *p5 = *(V8 *)D_801152A8;
            *p7 = p;
            *p8 = *(V8 *)&D_80126720;
            r |= 1;
            if ((p9 & 0x7F) != 0) {
                break;
            }
        }
    }
    return r;
}
