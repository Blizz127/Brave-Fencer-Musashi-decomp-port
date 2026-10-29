/* func_80169584 [80169584,801696D8) - owner re-derivation from retail assembly (clean input set).
 * Builds a gouraud colour record from o->f1C (two colour layouts by o->f2C), copies the
 * o translation to f4C..f54 and transforms two vertex sets with the object matrix at o+0x38.
 * Each arm stores r1/r2 itself; jump2 cross-jumps those identical tails into the join.
 * Callers read v0: none (void). */
#include "psx_types.h"

typedef struct { s16 x, y, z, w; } SV;
typedef struct {
    u8 r0, g0, b0, p0;
    u8 r1, g1, b1, p1;
    u8 r2, g2, b2, p2;
    u8 r3, g3, b3, p3;
    u32 w;
    u8 rest[0x2C];
} Prim;
typedef struct {
    u8 pad0[6];
    s16 f6;
    u8 pad8[2];
    s16 fA;
    u8 padC[2];
    s16 fE;
    u8 pad10[0xC];
    s32 f1C;
    u8 pad20[0xC];
    s32 f2C;
    u8 pad30[8];
    u8 f38[0x14];
    s32 f4C;
    s32 f50;
    s32 f54;
} Obj;

extern void func_80017758(SV *v, void *p);

void func_80169584(Obj *o) {
    SV v[4];
    Prim p;
    u8 c;
    u8 t;

    v[0].x = 7;
    v[3].x = -7;
    v[1].y = 2;
    v[2].x = 0;
    v[1].x = 0;
    v[3].y = 0;
    v[0].y = 0;
    v[2].y = -2;
    v[3].z = 0;
    v[2].z = 0;
    v[1].z = 0;
    v[0].z = 0;
    p.b0 = 0;
    p.r0 = 0;
    p.b3 = 0;
    p.r3 = 0;
    p.g3 = 0;
    p.g0 = 0;
    if (o->f2C == 0) {
        c = -0x40 - (o->f1C << 4);
        t = c >> 2;
        p.g2 = c;
        p.g1 = c;
        p.b2 = t;
        p.b1 = t;
        p.r2 = t;
        p.r1 = t;
    } else {
        c = -0x40 - (o->f1C << 4);
        t = c >> 2;
        p.g2 = c;
        p.g1 = c;
        p.b2 = c;
        p.b1 = c;
        p.r2 = t;
        p.r1 = t;
    }
    p.w = 0x50000000;
    o->f4C = o->f6;
    o->f50 = o->fA;
    o->f54 = o->fE;
    func_80017758(v, o->f38);
    v[0].y = 7;
    v[3].y = -7;
    v[1].x = 2;
    v[2].y = 0;
    v[1].y = 0;
    v[3].x = 0;
    v[0].x = 0;
    v[2].x = -2;
    func_80017758(v, o->f38);
}
