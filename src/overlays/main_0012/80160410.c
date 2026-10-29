/* func_80160410 [80160410,80160530) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x12];
    u16 f12;
} S;

typedef struct {
    u8 p0[0x20];
    S *f20;
    u8 p24[0x1E];
    u16 f42;
    u8 p44[0x9C];
    s16 fE0;
    s16 fE2;
    s16 fE4;
} O;

/* callers read v0: none-direct (callers tail-return a later call's result); void. f20 holds a guest address.
 * func_80146A6C: owner body (80146a6c.c) is s32 (s16, s32, s16, s16, u16, s32, s32); retail passes the
 * fifth arg from an lh (sign-extended), so the local prototype keeps s32 x/y/z and void. */
extern void func_80019064(void *p);
extern void func_80149020(O *o);
extern void func_80013E94(void *a, void *b);
extern void func_80146A6C(s32 a, O *o, s32 x, s32 y, s32 z, u32 c1, u32 c2);
extern void func_80147324(s32 a);
extern void func_8001382C(s32 a, void *p, void *q);
extern void func_80146DB8(O *o, void *p);
extern void func_80146E90(O *o, s32 a);
extern void func_80146CA0(O *o);
extern void func_801553A8(O *o);
extern s32 func_80149184(O *o);
extern u8 D_80062BD0[];
extern u16 D_801152A0;

static inline void call6b(s32 a, O *o, s32 x, s32 y, s32 z, u32 c) {
    func_80146A6C(a, o, x, y, z, c, 0);
}


void func_80160410(O *o) {
    s32 q[4];
    s32 r[12];
    u32 w[2];
    s16 s;
    u32 v;

    func_80019064(D_80062BD0);
    func_80149020(o);
    s = o->f42 - o->f20->f12;
    func_80013E94((u8 *)o + 0xE8, w);
    v = w[0];
    call6b(6, o, o->fE0, o->fE2, o->fE4, w[0] = v | 0x80000000);
    func_80147324(0x43D);
    q[0] = 0;
    q[1] = 0;
    q[2] = 0xFFFA0000;
    func_8001382C(s, q, r);
    func_80146DB8(o, r);
    q[0] = 0;
    q[1] = 0;
    q[2] = 0x8000;
    func_8001382C(s, q, (u8 *)o + 0x234);
    func_80146E90(o, 8);
    func_801553A8(o);
    D_801152A0 = func_80149184(o);
    func_80146CA0(o);
}
