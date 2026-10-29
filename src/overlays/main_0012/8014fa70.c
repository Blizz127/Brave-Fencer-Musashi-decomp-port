#include "psx_types.h"

typedef struct { u16 x; u16 y; u16 z; union { u16 w; u8 c; } u; } V8;
typedef struct { u8 b[8]; } B8;
typedef struct {
    u8 pad0[6];
    u16 x6;
    u8 pad8[2];
    u16 yA;
    u8 padC[2];
    u16 zE;
    u8 pad10[0x78];
    u16 f88;
    u16 f8A;
    u16 f8C;
    u8 pad8E[0xDE];
    u16 f16C;
    u16 f16E;
} Obj;

extern u8 D_801152A8[];
extern s16 D_801152B0;
extern s32 D_801152BC;
extern s32 func_80133784(s32 k, V8 *p, V8 *q);
extern u8 func_80029178(s32 a);

s32 func_8014FA70(Obj *o) {
    V8 a;
    V8 b;
    s32 r;

    a.x = o->f88;
    a.y = o->f8A;
    a.z = o->f8C;
    b.x = o->x6;
    b.y = o->yA;
    b.z = o->zE;
    r = func_80133784(1, &a, &b);
    if (r != 0) {
        o->x6 = b.x;
        o->yA = b.y;
        o->zE = b.z;
        if (b.u.c != 0) {
            o->f16C = b.u.c;
        }
        o->f16E = b.u.w >> 13;
        if (o->f16C == 0x1D) {
            if (func_80029178(0x21) == 0) {
                r &= ~0x2000;
                r |= 0x4000;
                *(B8 *)&D_801152B0 = *(B8 *)((s16 *)&D_801152A8);
                D_801152BC = 1;
            }
        }
        return r;
    }
    if (b.u.c != 0) {
        o->f16C = b.u.c;
    }
    return 0;
}
