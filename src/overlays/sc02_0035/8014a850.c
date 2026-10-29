/* Owner re-derivation from retail assembly (clean input set). */
/* Scans the slot pool for an active kind-7 slot within 0x40 of the object (y offset 0x40); on a hit records it and requests a handler. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u8 p2[4];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0x48];
    u32 f58;
    u16 f5C;
    u8 p5E[0xC1 - 0x5E];
    u8 fC1;
    u8 pC2[0x10C - 0xC2];
} Slot;

typedef struct {
    u8 p0[4];
    u8 f4[2];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0x10];
    u8 *f20;
    u8 p24[0x168];
    u32 f18C;
} O;

extern u8 D_801202A0[];
extern void func_80015978(void *src, void *dst);
extern s32 func_80029AF4(void);
extern void func_8014C4AC(Slot *p, s32 a, s32 b, s16 *c, s32 d);

s32 func_8014A850(O *o) {
    u32 i;
    Slot *p;
    s16 v[3];
    s16 w[4];
    s32 r;
    for (i = 0; i < 0x60; i++) {
        p = &((Slot *)&D_801202A0)[i];
        if (p->f0 != 0 && p->f58 != 0 && (p->f5C & 0x8000) && p->fC1 == 7) {
            v[0] = o->f6 - p->f6;
            v[1] = o->fA - (p->fA + 0x40);
            v[2] = o->fE - p->fE;
            if ((v[0] < 0 ? -v[0] : v[0]) < 0x40 && (v[2] < 0 ? -v[2] : v[2]) < 0x40 &&
                (v[1] < 0 ? -v[1] : v[1]) < 0x40) {
                o->f18C = (u32)p;
                func_80015978(o->f4, w);
                w[3] = 0;
                r = func_80029AF4();
                func_8014C4AC(p, 0x1B, r, w, *(s16 *)(o->f20 + 0x12));
                return 1;
            }
        }
    }
    return 0;
}
