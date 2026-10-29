/* Owner re-derivation from retail assembly (clean input set). */
/* Picks the flagged pool slot with the smallest heading error, eases the +0x20 object's angle toward it and (when the state flag matches) aims its +0x10 angle at the slot's box centre. */
#include "psx_types.h"

typedef struct {
    u8 p0[6];
    u16 u6;
    u8 p8[2];
    s16 sA;
    u8 pC[2];
    u16 uE;
    u8 p10[0x48];
    u32 f58;
    u16 f5C;
    u8 p5E[0x10C - 0x5E];
} Slot;

typedef struct {
    u8 p0[6];
    u16 u6;
    u8 p8[2];
    s16 sA;
    u8 pC[2];
    u16 uE;
    u8 p10[0x10];
    u8 *f20;
} O;

extern u8 D_801202A0[];
extern u8 D_800AF630[];
extern s16 func_80012A60(s16 a, s16 b);
extern s32 func_80012DBC(s32 from, s16 to, s16 num, s16 den);
extern s32 func_80047D3C(s32 v);
extern s32 func_8004CFEC(s32 a, s32 b);
extern s32 func_8014C59C(void *o, void *p);

void func_80164530(O *o) {
    s32 a;
    s32 b;
    u8 *g = D_800AF630;
    Slot *p;
    Slot *bs;
    u8 *q = o->f20;
    s32 bc;
    s16 r;
    s16 best = 0x1000;
    s16 dx;
    s16 dz;
    s16 dist;
    s32 cx;
    s32 cz;
    s32 cy;
    for (p = ((Slot *)&D_801202A0); p < &((Slot *)&D_801202A0)[96]; p++) {
        if (p->f5C & 0x2000) {
            a = func_8014C59C(o, p);
            r = func_80012A60(a, *(s16 *)(q + 0x12));
            if (r < 0x200 && r < best) {
                bc = a;
                best = r;
                bs = p;
            }
        }
    }
    if (best != 0x1000) {
        *(s16 *)(o->f20 + 0x12) = func_80012DBC(*(s16 *)(q + 0x12), bc, 0xA, 1);
        if (*(s16 *)(g + 0xA3D8) == 0x30A0) {
            b = bs->f58 & 0xFFFFFFF;
            if (b != 0) {
                cx = (((s16 *)b)[6] + ((s16 *)b)[7]) / 2;
                dx = bs->uE + cx - o->uE;
                cz = (((s16 *)b)[2] + ((s16 *)b)[3]) / 2;
                dz = bs->u6 + cz - o->u6;
                dist = func_80047D3C(dx * dx + dz * dz);
                cy = (((s16 *)b)[4] + ((s16 *)b)[5]) / 2;
                a = func_8004CFEC(bs->sA + cy - o->sA, dist) & 0xFFF;
                *(s16 *)(o->f20 + 0x10) = a;
            }
        }
    }
}
