/* func_80169408 [80169408,80169518) - owner re-derivation from retail assembly (clean input set). */
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
    u8 p10[2];
    u16 f12;
    u8 p14[2];
    u16 f16;
    u8 p18[2];
    u16 f1A;
    u8 p1C[0x10];
    u32 f2C;
} O;

/* callers read v0: none-direct (no jal caller); void.
 * func_80169584 receives o (retail loads it into a3 from a0); an owner source declares it void f(void). */
extern u32 func_8005C49C(void);
extern void func_80049A1C(void *a, void *b);
extern void func_80048EAC(void *a, void *b);
extern void func_80048FBC(void *a, void *b, void *c);
extern void func_800D20C0(void *a, void *b, s32 n);
extern void func_800D23D0(void *a);
extern void func_80169584(O *o);

void func_80169408(O *o) {
    u16 a[4];
    u16 c[4];
    s32 b[8];
    s32 pad[8]; /* unreferenced: retail frame keeps 32 bytes above the matrix */
    u8 *s;
    u32 r;

    if (o->f2C == 0) {
        s = (u8 *)o + 0x38;
        r = func_8005C49C();
        a[0] = (r & 0x7F) << 5;
        a[1] = (r & 0x7F00) >> 3;
        a[2] = 0;
        func_80049A1C(a, b);
        func_80048EAC(s, b);
        a[0] = 0;
        a[1] = 0;
        a[2] = 2;
        func_80048FBC(b, a, a);
        o->f12 = a[0];
        o->f16 = a[1];
        o->f1A = a[2];
        a[0] = o->f6;
        a[1] = o->fA;
        a[2] = o->fE;
        func_800D20C0(a, c, 1);
        func_800D23D0(c);
        func_80049A1C(c, s);
    }
    func_80169584(o);
    o->f2 = o->f2 + 1;
}
