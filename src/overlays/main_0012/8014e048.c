/* Owner re-derivation from retail assembly (clean input set). */
/* Validates the tracked slot (o->f170) against the move from a to b; on success either snaps the slot to the probed point or moves the object, otherwise clears the link. Returns 1 on success. */
#include "psx_types.h"

typedef struct {
    u16 x;
    u16 y;
    u16 z;
    u16 w;
} V8;

typedef struct {
    u16 f0;
    u8 p2[4];
    u16 u6;
    u8 p8[2];
    u16 uA;
    u8 pC[2];
    u16 uE;
    u8 p10[0x10];
    u32 f20;
    u8 p24[0x34];
    u32 f58;
    u16 f5C;
} Slot;

typedef struct {
    u8 p0[6];
    u16 u6;
    u8 p8[2];
    u16 uA;
    u8 pC[2];
    u16 uE;
    u8 p10[0x160];
    u32 f170;
} O;

extern u8 D_801152A8[];
extern s16 func_80012A60(s16 a, s16 b);
extern s32 func_8004CFEC(s32 a, s32 b);
extern s32 func_80133784(s32 a, void *p, void *q);
extern s32 func_80135A4C(u32 shape, u32 kind, void *probe, void *out);

s32 func_8014E048(O *o, u16 *a, u16 *b) {
    Slot *q;
    s16 dx;
    s16 dz;
    s16 ex;
    s16 ey;
    u16 px;
    u16 py;
    u16 pz;
    s32 r1;
    s32 r;
    s16 v[3];
    V8 w;
    dx = b[0] - a[0];
    dz = b[2] - a[2];
    if ((dx | dz) != 0) {
        w.x = dx;
        w.y = b[1] - a[1];
        w.z = b[2] - a[2];
        w.x = -dx;
        w.z = -w.z;
        q = (Slot *)o->f170;
        if (q->f0 != 0 && (q->f5C & 0x1000) && q->f58 != 0 && func_80135A4C(q->f20, q->f58, a, b) != 0) {
            ex = q->u6 - o->u6;
            ey = q->uE - o->uE;
            r1 = (s16)func_8004CFEC(dz, dx);
            if (func_80012A60(r1, func_8004CFEC(ey, ex)) < 0x400) {
                v[0] = px = q->u6;
                v[1] = py = q->uA;
                v[2] = pz = q->uE;
                w.x = px + w.x;
                w.y = py + w.y;
                w.z = pz + w.z;
                r = func_80133784(0, v, &w);
                if (r == 0 || ((r & 0x2000) && ((*(V8 *)&D_801152A8).x | (*(V8 *)&D_801152A8).z) == 0)) {
                    q->u6 = w.x;
                    q->uA = w.y;
                    q->uE = w.z;
                } else {
                    o->u6 = a[0];
                    o->uA = a[1] + 0x10;
                    o->uE = a[2];
                }
                return 1;
            }
        }
    }
    o->f170 = 0;
    return 0;
}
