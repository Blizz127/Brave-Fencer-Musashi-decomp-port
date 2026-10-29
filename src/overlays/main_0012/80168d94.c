/* func_80168D94 [80168D94,80168DFC+) - owner re-derivation from retail assembly (clean input set). */
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
    u16 f10;
    u16 f12;
    u16 f14;
    u16 f16;
    u16 f18;
    u16 f1A;
    u32 f1C;
    u8 p20[0xC];
    u32 f2C;
} O;

/* callers read v0: none-direct (no jal caller); void. */
extern u32 func_8005C49C(void);
extern void func_80049A1C(void *a, void *b);
extern void func_80048EAC(void *a, void *b);
extern void func_80048FBC(void *a, void *b, void *c);

void func_80168D94(O *o) {
    u16 a[4];
    s32 b[8];
    s32 m[8];
    u16 t;

    a[0] = o->f12;
    a[1] = o->f16;
    a[2] = o->f1A;
    func_80049A1C(a, m);
    if (o->f2C == 0) {
        u32 r = func_8005C49C();
        a[0] = (r & 0x7F) << 3;
        a[1] = ((r & 0x7F00) >> 5) + 0xC00;
    } else {
        u32 r = func_8005C49C();
        a[0] = ((r & 0x7F) << 3) + 0xE00;
        a[1] = ((r & 0x7F00) >> 5) + 0xE00;
    }
    a[2] = 0;
    func_80049A1C(a, b);
    func_80048EAC(m, b);
    a[0] = 0;
    a[1] = 0;
    a[2] = 0x24;
    func_80048FBC(b, a, a);
    {
    u16 bA = o->fA;
    *(u16 *)((u8 *)o + 0x12) = a[0];
    *(u16 *)((u8 *)o + 0x16) = a[1];
    t = a[2];
    o->f10 = o->f6;
    o->f14 = bA;
    o->f18 = o->fE;
    o->f1C = 0;
    o->f2 = o->f2 + 1;
    o->f1A = t;
    }
}
