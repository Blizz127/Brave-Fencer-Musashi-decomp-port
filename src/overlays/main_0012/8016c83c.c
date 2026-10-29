#include "psx_types.h"

typedef struct { s16 x, y, z, w; } SV;
typedef union { s32 w; struct { u16 lo, hi; } h; } W;
typedef struct { u8 pad0[6]; u16 f6; u8 pad8[2]; u16 fA; u8 padC[2]; u16 fE; } Q;
typedef struct {
    u8 pad0[2];
    u16 f2;
    u8 pad4[2];
    u16 f6;
    u8 pad8[2];
    u16 fA;
    u8 padC[2];
    u16 fE;
    W v[3];
    s32 f1C;
    u8 pad20[0x14];
    Q *f34;
} Obj;

extern s32 func_80047E98(SV *a, SV *b);
extern void func_800D22E4(Obj *o);
extern void func_8016CF04(Obj *o, s32 a);

void func_8016C83C(Obj *o) {
    Q *q;
    SV v;
    s32 pad[8];
    s32 r;
    s32 lim;
    s32 a;

    q = o->f34;
    o->f1C = o->f1C - 1;
    if (o->f1C == 0) {
        o->f2 = 4;
        func_8016CF04(o, 2);
    } else {
        v.x = q->f6 - o->f6;
        v.y = q->fA - o->fA;
        v.z = q->fE - o->fE;
        if (func_80047E98(&v, &v) < 0x790) {
            o->f2 = 4;
            func_8016CF04(o, 2);
        } else {
            o->v[0].w = o->v[0].w + (v.x << 6);
            o->v[1].w = o->v[1].w + (v.y << 6);
            o->v[2].w = o->v[2].w + (v.z << 6);
            v.x = o->v[0].h.hi;
            v.y = o->v[1].h.hi;
            v.z = o->v[2].h.hi;
            r = func_80047E98(&v, &v);
            lim = 0x1000;
            if (r > lim) {
                o->v[0].w = v.x << 10;
                o->v[1].w = v.y << 10;
                o->v[2].w = v.z << 10;
            }
            func_800D22E4(o);
            func_8016CF04(o, 1);
        }
    }
    
}





