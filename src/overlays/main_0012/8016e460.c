#include "psx_types.h"

typedef struct { s16 x, y, z, w; } SV;
typedef struct { u16 x, y, z, w; } UV;
typedef struct { s32 w[8]; } M8;
typedef struct {
    u8 r0, g0, b0, p0;
    u8 r1, g1, b1, p1;
    u8 r2, g2, b2, p2;
    u8 r3, g3, b3, p3;
    u32 w;
    u8 rest[4];
} Prim;
typedef struct {
    u8 pad0[6];
    s16 f6;
    u8 pad8[2];
    s16 fA;
    u8 padC[2];
    s16 fE;
    u8 pad10[0x1C];
    u16 f2C;
    u8 pad2E[2];
    u8 f30;
} Obj;

extern void func_80017DC4(UV *v, M8 *m);
extern void func_80048EAC(void *a, M8 *m);
extern void func_80017758(SV *v, M8 *m);

void func_8016E460(Obj *o, void *a1) {
    SV v[4];
    Prim p;
    M8 m;
    UV u;
    u8 c;
    u8 t;
    u8 s;
    u16 w;

    v[0].x = 7;
    v[2].x = 0;
    v[1].x = 0;
    v[3].x = -7;
    v[1].y = 3;
    v[3].y = 0;
    v[0].y = 0;
    v[2].y = -3;
    v[3].z = 0;
    v[2].z = 0;
    v[1].z = 0;
    v[0].z = 0;
    c = o->f30;
    p.w = 0x50000000;
    t = c >> 2;
    s = t >> 2;
    p.b2 = c;
    p.b1 = c;
    p.r2 = c;
    p.r1 = c;
    p.g2 = t;
    p.g1 = t;
    p.r3 = s;
    p.r0 = s;
    p.b3 = s;
    p.b0 = s;
    p.g3 = s;
    p.g0 = s;
    w = o->f2C;
    u.y = w;
    u.x = w;
    func_80017DC4(&u, &m);
    func_80048EAC(a1, &m);
    m.w[5] = o->f6;
    m.w[6] = o->fA;
    m.w[7] = o->fE;
    func_80017758(v, &m);
    v[0].x = 3;
    v[2].x = 0;
    v[1].x = 0;
    v[3].x = -3;
    v[1].y = 7;
    v[3].y = 0;
    v[0].y = 0;
    v[2].y = -7;
    func_80017758(v, &m);
}
