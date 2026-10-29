/* Owner re-derivation from retail assembly (clean input set). */
/* Same probe as func_8014F4C0 without the early +0x146 exit and without the +8 lift; a hit moves the object to the probed point; a miss returns 0x8000 when the probe point equals the wanted point, else 0. */
#include "psx_types.h"

typedef struct {
    u16 x;
    u16 y;
    u16 z;
    u16 w;
} V8;

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    union {
        u16 h;
        u8 b;
    } w;
} W;

typedef struct {
    u8 p0[6];
    u16 u6;
    u8 p8[2];
    u16 uA;
    u8 pC[2];
    u16 uE;
    u8 p10[0x78];
    u16 u88;
    u16 u8A;
    u16 u8C;
    u8 p8E[0x120 - 0x8E];
    V8 f120;
    u8 p128[0x140 - 0x128];
    u16 u140;
    u16 u142;
    u16 u144;
    s16 s146;
    u8 p148[0x16C - 0x148];
    u16 u16C;
    u16 u16E;
} O;

extern u8 D_801152A8[];
extern s16 D_801152B0;
extern s32 D_801152BC;
extern u8 D_800D3918[];
extern u8 func_80029178(s32 a);
extern s32 func_80133784(s32 a, s16 *p, V8 *q);

s32 func_8014F74C(O *o) {
    s16 v[3];
    W w;
    s16 u[3];
    s32 r;
    v[0] = o->u88;
    v[1] = o->u8A;
    v[2] = o->u8C;
    w.x = u[0] = o->u6;
    w.y = u[1] = o->uA;
    w.z = u[2] = o->uE;
    if (o->s146 != 0) {
        w.x += o->u140;
        w.y += o->u142;
        w.z += o->u144;
    }
    r = func_80133784(0, v, (V8 *)&w);
    if (r != 0) {
        o->u6 = w.x;
        o->uA = w.y;
        o->uE = w.z;
        if (w.w.b != 0) {
            o->u16C = w.w.b;
        }
        o->u16E = w.w.h >> 13;
        if (r == 0x2000) {
            o->f120 = (*(V8 *)&D_801152A8);
        } else if (r == 0x4000) {
            o->f120 = *(V8 *)&D_801152B0;
        }
        if (o->u16C == 0x1D) {
            if (func_80029178(0x21) == 0) {
                r &= ~0x2000;
                r |= 0x4000;
                *(V8 *)&D_801152B0 = (*(V8 *)&D_801152A8);
                D_801152BC = 1;
            }
        }
        return r;
    }
    o->f120 = *(V8 *)((char *)&D_800D3918);
    if (w.w.b != 0) {
        o->u16C = w.w.b;
    }
    if (v[0] != u[0] || v[1] != u[1] || v[2] != u[2]) {
        if (v[0] == w.x && v[1] == w.y && v[2] == w.z) {
            return 0x8000;
        }
    }
    return 0;
}
