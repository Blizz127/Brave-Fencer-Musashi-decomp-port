#include "psx_types.h"

typedef struct { u16 x; u16 y; u16 z; u16 w; } V8;
typedef struct { u8 pad[0x18]; u16 f18; u16 f1A; } Sub;
typedef struct {
    u8 pad0[6];
    u16 x6;
    u8 pad8[2];
    u16 yA;
    u8 padC[2];
    u16 zE;
    u8 pad10[2];
    u16 f12;
    u8 pad14[2];
    u16 f16;
    u8 pad18[2];
    u16 f1A;
    s32 f1C;
    Sub *p20;
} Obj;

extern void func_80146C3C(Obj *o);
extern s32 func_80146A6C(s32 a, Obj *o, s32 x, s32 y, s32 z, u32 c1, u32 c2);
extern void func_800D20C0(V8 *a, V8 *b, s32 n);

void func_8016D848(Obj *o) {
    V8 a;
    V8 b;
    s16 i;
    s16 d;
    Sub *p;
    u16 u, w;

    o->f1C = o->f1C - 1;
    if (o->f1C == 0) {
        func_80146C3C(o);
    } else {
        if (o->f1C < 4) {
            d = -0xC00;
        } else {
            d = 0x800;
            for (i = 0; i < 5; i++) {
                func_80146A6C(0x33, o, 0, 0, 0, 0x8000, 0);
            }
        }
        p = o->p20;
        if (d == 0) {
            u = p->f18 ^ 0x400;
            w = p->f1A ^ 0x200;
        } else {
            u = p->f18 + d;
            w = p->f1A + d;
        }
        p->f18 = u;
        p->f1A = w;
        a.x = o->f12;
        a.y = o->f16;
        a.z = o->f1A;
        func_800D20C0(&a, &b, 6);
        o->x6 = a.x;
        o->yA = a.y;
        o->zE = a.z;
    }
}
