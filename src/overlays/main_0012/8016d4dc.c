/* func_8016D4DC [8016D4DC,8016D5E4) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u8 p2[4];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
} T;

typedef struct {
    u8 p0[2];
    u16 f2;
    u8 p4[2];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0xC];
    u32 f1C;
    u8 p20[0x14];
    T *f34;
} O;

/* callers read v0: none-direct (no jal caller); void.
 * f34 holds a guest address (T *). */
extern void func_8016C83C(O *o);
extern s32 func_80047E98(void *in, void *out);
extern void func_8016CF04(O *o, s32 a);
extern void func_8016C74C(O *o, void *v, s32 a, s32 b, s32 c);
extern void func_800D22E4(O *o);
extern u8 D_80126B58[];

void func_8016D4DC(O *o) {
    u16 v[4];
    s32 pad[8]; /* unreferenced: retail frame keeps 32 bytes above the vector */
    T *t;
    s32 d;
    s32 k;

    t = o->f34;
    if (t->f0 != 1) {
        o->f2 = 3;
        o->f34 = (T *)D_80126B58;
        o->f1C = 0x40;
        func_8016C83C(o);
        return;
    }
    v[0] = t->f6 - o->f6;
    v[1] = t->fA - o->fA;
    v[2] = t->fE - o->fE;
    d = func_80047E98(v, v);
    if (d < 0x510) {
        func_8016CF04(o, 2);
        o->f2 = 4;
        return;
    }
    if (d < 0x1900) {
        k = 6;
    } else {
        k = 4;
        if (d < 0x6400) {
            k = 5;
        }
    }
    func_8016C74C(o, v, k, 9, 0x400);
    func_800D22E4(o);
    func_8016CF04(o, 1);
}
