#include "psx_types.h"

typedef struct { u16 x; u16 y; u16 z; u16 w; } V8;
typedef struct {
    u16 id;
    u8 pad2[8];
    s16 fA;
    u8 padC[0x14];
    s32 f20;
    u8 pad24[0x34];
    s32 f58;
    u16 f5C;
    u8 pad5E[0x17];
    u8 f75;
    u8 pad76[0x10C - 0x76];
} Slot; /* 0x10C */

typedef struct {
    u8 pad0[6];
    u16 x6;
    u8 pad8[2];
    s16 yA;
    u8 padC[2];
    u16 zE;
    u8 pad10[0x160];
    Slot *f170;
    u8 pad174[4];
    Slot *f178;
    u8 pad17C[0x0];
} Obj;

extern u8 D_801202A0[];
extern s32 func_80135A4C(s32 shape, s32 kind, V8 *probe, V8 *out);
extern s32 func_8014C918(Obj *o, s32 k);

s32 func_8014D2A0(Obj *o, V8 *a, V8 *b) {
    Slot *p;
    for (p = ((Slot *)&D_801202A0); p < &((Slot *)&D_801202A0)[96]; p++) {
        if (p->id != 0 && (p->f5C & 0x400) && p->f58 != 0 && p != o->f178 && p != o->f170 && p->fA >= o->yA) {
            if (func_80135A4C(p->f20, p->f58, a, b) != 0) {
                o->x6 = b->x;
                o->yA = b->y;
                o->zE = b->z;
                *(s16 *)((u8 *)o + 0x16E) = func_8014C918(o, p->f75) & 0xFF;
                return 1;
            }
        }
    }
    return 0;
}
