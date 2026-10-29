/* Owner re-derivation from retail assembly (clean input set). */
/* func_80144558 [80144558,8014462C) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[2];
    u16 f2;
    u8 p4[2];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0xBC];
    u32 fCC;
} O;

typedef struct {
    u8 p0[4];
    u32 f4;
    u16 f8;
    u16 fA;
    u16 fC;
    u8 pE[0xA];
    u16 f18;
    u16 f1A;
    u8 p1C[4];
    u32 f20;
    u8 p24[3];
    u8 f27;
} N;

/* callers read v0: none-direct (no jal caller); void. N is the new object (a guest pointer in o->fCC).
 * func_80128EA8 is declared with void* here (owner: Obj*, Dst*, Src*). */
extern N *func_8012C194(void);
extern void func_8012CAE4(char *o);
extern void func_8001CC3C(u16 *h, s32 a, s32 b, s32 c);
extern void func_80128EA8(void *o, void *d, void *s);
extern void func_8012B200(void *o);
extern u8 D_800D387C[];
extern u8 D_800D3888[];

void func_80144558(O *o) {
    N *n;

    n = func_8012C194();
    if (n == 0) {
        func_8012CAE4((char *)o);
    } else {
        o->fCC = (u32)n;
        func_8001CC3C((u16 *)n, 0, 0, 0);
        n->f20 = (u32)D_800D387C;
        n->f27 = 0x9C;
        n->f1A = 0x3000;
        n->f18 = 0x3000;
        n->f4 |= 0x50000000;
        func_80128EA8(n, (u8 *)o + 0xD0, D_800D3888);
        n->f8 = o->f6;
        n->fA = o->fA;
        n->fC = o->fE;
        o->f2 = 1;
        func_8012B200(o);
    }
}
