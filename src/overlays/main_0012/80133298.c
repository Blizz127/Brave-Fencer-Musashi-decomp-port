/* func_80133298 [80133298,80133404) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x20];
    u32 f20;
} O;
typedef struct { s32 w[8]; } M;

/* callers read v0: none-direct (no jal caller); void. */
extern void func_8012F0BC(u32 p, s32 *a, s32 *b);
extern void func_8012B2CC(O *o);
extern void func_8013339C(M *m, u32 p);
extern void func_8012F1A4(M *m, s32 *b, s32 *c);
extern s32 D_80126B5C;
extern s32 D_80126B60;
extern s32 D_80126B64;

void func_80133298(O *o) {
    s32 a[4];
    s32 c[4];
    s32 b[4];
    M m;

    a[0] = D_80126B5C;
    a[1] = D_80126B60;
    a[2] = D_80126B64;
    func_8012F0BC(o->f20 + 0x34, a, b);
    func_8012B2CC(o);
    m = *(M *)(o->f20 + 0x34);
    func_8013339C(&m, o->f20 + 0x18);
    func_8012F1A4(&m, b, c);
    D_80126B5C = c[0];
    D_80126B60 = c[1];
    D_80126B64 = c[2];
}
