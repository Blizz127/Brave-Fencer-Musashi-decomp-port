/* func_8016D778 [8016D778,8016D848) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u8 p0[0x18]; u16 f18; u16 f1A; } Q;
typedef struct { u8 p0[0x36]; s16 f36; } R;
typedef struct {
    u8 p0[6];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[2];
    u16 f12;
    u8 p14[2];
    u16 f16;
    u8 p18[2];
    u16 f1A;
    u8 p1C[4];
    Q *f20;
    u8 p24[4];
    s16 f28;
    u8 p2A[6];
    R *f30;
} O;

/* callers read v0: none-direct (no jal caller); void. */
extern void func_80146C3C(O *o);
extern void func_800D20C0(u16 *a, u16 *b, s32 n);
extern u32 D_80126B9C;

void func_8016D778(O *o) {
    u16 v[4];
    u16 w[4];
    Q *q;

    if (o->f28 != o->f30->f36 || (D_80126B9C & 0x20) == 0) {
        func_80146C3C(o);
    } else {
        q = o->f20;
        q->f18 ^= 0x400;
        q->f1A ^= 0x200;
        v[0] = o->f12;
        v[1] = o->f16;
        v[2] = o->f1A;
        func_800D20C0(v, w, 6);
        o->f6 = v[0];
        o->fA = v[1];
        o->fE = v[2];
    }
}
