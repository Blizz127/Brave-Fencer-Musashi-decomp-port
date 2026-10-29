#include "psx_types.h"

typedef struct { s32 w[8]; } M8;
typedef struct { u16 x, y, z, w; } V8;
typedef struct { u8 b0, b1, b2; s8 b3, b4, b5; u16 h6; } R8;
typedef struct { s16 a, b, c; u16 h6, h8, hA; } R12;
typedef struct {
    u8 pad0[0x20];
    u32 f20;
    u8 pad24[0x10];
    M8 m34;
} Sub;
typedef struct {
    u8 pad0[0x20];
    Sub *p20;
} Obj;

extern void func_80049CAC(V8 *v, M8 *out);

void func_8012EA90(Obj *o, s32 idx, M8 *out) {
    V8 v;
    Sub *sub;
    R8 *r;
    R12 *q;
    s32 w;
    s32 pad[2];

    sub = o->p20;
    q = (R12 *)sub->f20;
    if (q == 0) {
        *out = sub->m34;
        return;
    }
    if ((u32)q & 0x01000000) {
        r = (R8 *)((u32)q & 0xFEFFFFFF) + idx;
        v.x = r->h6;
        w = *(u32 *)r;
        v.y = r->b1 | ((w & 0xF) << 8);
        v.z = ((w >> 16) & 0xFF) | ((w & 0xF0) << 4);
        func_80049CAC(&v, out);
        out->w[5] = r->b3;
        out->w[6] = r->b4;
        out->w[7] = r->b5;
        return;
    }
    q = q + idx;
    v.x = q->h6;
    v.y = q->h8;
    v.z = q->hA;
    func_80049CAC(&v, out);
    out->w[5] = q->a;
    out->w[6] = q->b;
    out->w[7] = q->c;
}
