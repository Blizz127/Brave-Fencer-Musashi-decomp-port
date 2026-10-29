/* Owner re-derivation from retail assembly (clean input set). */
/* Searches the 0x60-entry slot pool for an active slot whose test passes and whose direction is within range of the offset; records it and the target. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u8 p2[4];
    u16 f6;
    u8 p8[6];
    u16 fE;
    u8 p10[0x10];
    u32 f20;
    u8 p24[0x34];
    u32 f58;
    u16 f5C;
    u8 p5E[0x10C - 0x5E];
} Slot;

extern u8 D_801202A0[];
extern s16 func_80012A60(s16 a, s16 b);
extern s32 func_8004CFEC(s32 a, s32 b);
extern s32 func_80135A4C(u32 shape, u32 kind, void *probe, void *out);

s32 func_8014DD8C(u8 *o, u16 *p1, u16 *p2) {
    Slot *p;
    s16 dx;
    s16 dz;
    s16 ex;
    s16 ey;
    s32 r1;
    dx = p2[0] - p1[0];
    dz = p2[2] - p1[2];
    if ((dx | dz) != 0) {
        for (p = ((Slot *)&D_801202A0); p < &((Slot *)&D_801202A0)[96]; p++) {
            if (p->f0 != 0 && p->f58 != 0 && (p->f5C & 0x1000) &&
                func_80135A4C(p->f20, p->f58, p1, p2) != 0) {
                ex = p->f6 - *(u16 *)(o + 6);
                ey = p->fE - *(u16 *)(o + 0xE);
                r1 = (s16)func_8004CFEC(dz, dx);
                if (func_80012A60(r1, func_8004CFEC(ey, ex)) < 0x400) {
                    *(u32 *)(o + 0x170) = (u32)p;
                    *(u16 *)(o + 6) = p2[0];
                    *(u16 *)(o + 0xA) = p2[1] + 0x10;
                    *(u16 *)(o + 0xE) = p2[2];
                    return 1;
                }
            }
        }
    }
    return 0;
}
