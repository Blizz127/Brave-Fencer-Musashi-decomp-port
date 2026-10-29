/* func_80166054 - owner re-derivation, clean room (nmO).
 * ang and sh are both s16 locals: the angle's sign extension is the s16
 * conversion at the call args (retail sll in each arm, sra at the join), and
 * the s16 types give global-alloc the retail order ang s0 > sh s1 > o s2.
 * r->h18 is stored in both arms with the constant arm first (nmL i).
 * callers read v0: none (void). */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    u32 w4;
    u8 pad8[0x10];
    u16 h18;
    u16 h1A;
} R;
typedef struct { u8 pad[0x12]; u16 f12; } Sub;
typedef struct { u8 pad[0x20]; Sub *p20; } Tgt;
typedef struct {
    u8 pad0[2];
    u16 f2;
    u8 pad4[0xC];
    s32 f10;
    s32 f14;
    s32 f18;
    u8 pad1C[4];
    R *f20;
    u8 pad24[8];
    s32 f2C;
    u32 f30;
    Tgt *f34;
} Obj;

extern u8 D_800D387C[];
extern u8 D_800D3888[];
extern void func_80149374(Tgt *t, void *p);
extern s32 func_8004787C(s32 a);
extern s32 func_80047948(s32 a);
extern void func_800D1FC8(Obj *o, s32 v);
extern R *func_800D21C4(Obj *o, u8 *a, s32 n);
extern void func_80128EA8(R *r, void *d, u8 *s);
extern void func_801661CC(Obj *o);
extern void func_80146C3C(Obj *o);

void func_80166054(Obj *o) {
    Tgt *t;
    s16 ang;
    s16 sh;
    s32 x;
    s32 vy;
    s32 y;
    s32 vx;
    R *r;

    t = o->f34;
    func_80149374(t, (u8 *)o + 4);
    if (o->f2C != 0) {
        sh = 8;
        if (o->f2C != 3) {
            ang = t->p20->f12;
        } else {
            ang = o->f30 & 0xFFF;
            sh = (o->f30 >> 12) & 0xF;
        }
        x = func_8004787C(ang) << sh;
        vx = -x;
        y = func_80047948(ang) << sh;
        vy = -y;
        if (o->f2C == 2) {
            vx = x;
            vy = y;
        }
        o->f10 = vx;
        o->f18 = vy;
    }
    o->f14 = 0xFFFF0000;
    func_800D1FC8(o, 6);
    r = func_800D21C4(o, D_800D387C, 0x9C);
    if (r != 0) {
        o->f20 = r;
        r->w4 = r->w4 | 0x50000040;
        if (o->f2C != 3) {
            r->h18 = 0x2000;
        } else {
            r->h18 = *(u16 *)((u8 *)o + 0x32);
        }
        r->h1A = r->h18;
        func_80128EA8(r, (u8 *)o + 0x24, D_800D3888);
        func_801661CC(o);
        o->f2 = o->f2 + 1;
        return;
    }
    func_80146C3C(o);
}
