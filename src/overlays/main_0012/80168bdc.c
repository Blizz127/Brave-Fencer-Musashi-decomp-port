#include "psx_types.h"

typedef struct { s32 w[8]; } M8;
typedef struct { u16 x; s16 y; s16 z; u16 w; } V8;
typedef struct {
    u8 pad0[0x12];
    u16 f12;
    u8 pad14[2];
    u16 f16;
    u8 pad18[2];
    u16 f1A;
} R;
typedef struct {
    u8 pad0[6];
    u16 f6;
    u8 pad8[2];
    u16 fA;
    u8 padC[2];
    u16 fE;
    u8 pad10[2];
    u16 f12;
    u8 pad14[2];
    u16 f16;
    u8 pad18[2];
    u16 f1A;
    u8 pad1C[0x10];
    s32 f2C;
} Obj;

extern void func_800D2318();
extern void func_80049A1C(V8 *v, M8 *m);
extern void func_80048FBC(M8 *m, V8 *a, V8 *b);
extern R *func_80146A6C(u16 a, Obj *o, s32 x, s16 y, s32 z, s32 c1, s32 c2);

void func_80168BDC(Obj *o, s32 a1, s16 a2, s16 a3) {
    M8 m;
    V8 v;
    s16 i;
    R *r;

    func_800D2318();
    v.x = o->f12;
    v.y = o->f16;
    v.z = 0;
    func_80049A1C(&v, &m);
    v.x = 0;
    if (o->f2C < 0) {
        v.z = -8;
        v.y = -4;
    } else {
        v.z = 2;
        v.y = -2;
    }
    func_80048FBC(&m, &v, &v);
    o->f6 = o->f6 + v.x;
    o->fA = o->fA + (u16)v.y;
    o->fE = o->fE + (u16)v.z;
    for (i = 0; i < a3; i++) {
        r = func_80146A6C(a1, o, (s16)o->f6, (s16)o->fA, (s16)o->fE, a2, 0);
        if (r != 0) {
            r->f12 = o->f12;
            r->f16 = o->f16;
            r->f1A = o->f1A;
        }
    }
}
