/* func_8012E014 [8012E014,8012E128) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[8];
    s16 f8;
    s16 fA;
    s16 fC;
    u8 pE[0x1E];
    u16 f2C;
    u8 p2E[0x1A];
    s32 f48;
    s32 f4C;
    s32 f50;
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
    u8 p24[0x2C];
    u16 f50;
    u16 f52;
    u16 f54;
} O;

/* callers read v0: none-direct; void. f20 holds a guest address. */
extern void func_8012F0BC(u8 *p, s32 *a, s32 *b);
extern void func_80049CAC(u8 *a, u8 *b);
extern void func_8012F1A4(u8 *p, s32 *b, s32 *c);
extern s32 D_80126B5C;
extern s32 D_80126B60;
extern s32 D_80126B64;

void func_8012E014(O *o) {
    s32 a[4];
    s32 c[4];
    s32 b[4];
    S *s;
    s16 x;
    s16 y;
    s16 z;

    a[0] = D_80126B5C;
    a[1] = D_80126B60;
    a[2] = D_80126B64;
    func_8012F0BC((u8 *)o->f20 + 0x34, a, b);
    if (o->f20 != 0) {
        func_80049CAC((u8 *)o->f20 + 0x10, (u8 *)o->f20 + 0x34);
        s = o->f20;
        x = o->f6 + o->f50;
        s->f8 = x;
        s->f48 = x;
        y = o->fA + o->f52;
        s->fA = y;
        s->f4C = y;
        z = o->fE + o->f54;
        s->f2C |= 1;
        s->fC = z;
        s->f50 = z;
    }
    func_8012F1A4((u8 *)o->f20 + 0x34, b, c);
    D_80126B5C = c[0];
    D_80126B60 = c[1];
    D_80126B64 = c[2];
}
