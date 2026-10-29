/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame step: wobbles the +0x12 angle between random bounds, rebuilds the +0x38 matrix and spawns/updates a linked effect. */
#include "psx_types.h"

typedef struct { u32 w[8]; } M32;
typedef struct {
    u8 p0[6];
    s16 s6;
    u8 p8[2];
    s16 sA;
    u8 pC[2];
    s16 sE;
    s16 s10;
    s16 s12;
    u8 p14[8];
    s32 f1C;
    u8 p20[0x10];
    s32 f30;
    u8 *f34;
    M32 m38;
} O;

extern u16 D_800B99DA;
extern void func_80048EAC(void *a, void *b);
extern void func_80049A1C(void *a, void *b);
extern s32 func_8005C49C(void);
extern s32 func_80146A6C(s32 a, void *o, s32 x, s16 y, s32 z, u32 c1, u32 c2);
extern void func_80146C3C(void *o);
extern void func_801670E4(u8 *o, s32 a, s32 b, s32 c);

void func_801678F0(O *o) {
    u8 *p;
    u8 *r;
    s16 v[3];
    p = o->f34;
    if (*(u32 *)(p + 0x44) & 0x10) {
        if (o->s12 > o->s10) {
            if (o->f1C > 0) {
                o->s10 = func_8005C49C() & 0x30;
                o->f1C = -0x30;
            }
        } else {
            if (o->f1C < 0) {
                o->s10 = (func_8005C49C() & 0x30) + 0x40;
                o->f1C = 0x30;
            }
        }
        o->s12 += o->f1C;
        if (o->f30 != 0) {
            o->f30 -= 0xC0;
        }
        v[0] = 0x400;
        v[1] = 0;
        v[2] = D_800B99DA << 8;
        func_80049A1C(v, &o->m38);
        func_80048EAC(*(u8 **)(p + 0x20) + 0x34, &o->m38);
        func_801670E4((u8 *)o, -6, -0x44, -0x18);
        r = (u8 *)func_80146A6C(0xE, o, o->s6, o->sA, o->sE, 0, 0);
        if (r != 0) {
            *(M32 *)(r + 0x38) = o->m38;
        }
    } else {
        func_80146C3C(o);
    }
}
