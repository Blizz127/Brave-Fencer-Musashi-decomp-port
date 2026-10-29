/* func_80164418 [80164418,80164530) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[6];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
} T;

typedef struct {
    u8 p0[0x10];
    s16 f10;
    s16 f12;
    u16 f14;
} S;

typedef struct {
    u8 p0[6];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0x10];
    S *f20;
    u8 p24[0x28];
    T *f4C;
} O;

/* callers read v0: none-direct; void. f20 and f4C hold guest addresses. */
extern s32 func_8004CFEC(s32 a, s32 b);
extern s32 func_80012ABC(s32 a, s32 b, s32 c);
extern void func_801630C4(O *o, s32 a, s32 b, s32 c);
extern void func_80162FC0(O *o);
extern void func_80162D88(O *o);

void func_80164418(O *o) {
    s16 d[4];
    s16 w[4];
    T *t;
    S *s;

    t = o->f4C;
    s = o->f20;
    d[0] = t->f6 - o->f6;
    d[1] = t->fA - (o->fA + 0x1E);
    d[2] = t->fE - o->fE;
    w[1] = func_8004CFEC(d[0], d[2]);
    if (d[2] < 0) {
        d[2] = -d[2];
    }
    if (d[2] == 0) {
        w[0] = 0;
    } else {
        w[0] = func_8004CFEC(-d[1], d[2]);
    }
    s->f14 = (s->f14 + 0x71) & 0xFFF;
    s->f10 = func_80012ABC(s->f10, w[0], 2);
    s->f12 = func_80012ABC(s->f12, w[1], 2);
    func_801630C4(o, 0, 0, 0x8000);
    func_80162FC0(o);
    func_80162D88(o);
}
