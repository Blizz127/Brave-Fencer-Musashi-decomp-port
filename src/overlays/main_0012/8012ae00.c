/* func_8012AE00 [8012AE00,8012AF0C) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u16 x, y, z; } V3;
typedef struct { u16 x, y, z, w; } V4;

typedef struct {
    u8 p0[6];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0x2A];
    u16 f3A;
    u8 p3C[2];
    u16 f3E;
    u8 p40[2];
    u16 f42;
} O;

/* callers read v0: none-direct (no jal caller); returns 1/0.
 * o+4 and o+0x38 are 12-byte word vectors (copied word by word). */
extern s32 func_80135888(u32 obj, u32 flags, void *from, void *to);
extern s32 func_80013478(void *a, void *b);
extern u32 D_80126B78;
extern u32 D_80126B90;
extern u8 D_80126B58[];

s32 func_8012AE00(O *o) {
    V3 a;
    V4 b;
    V4 c;
    s32 t;
    s32 u;
    u8 *p;

    a.x = o->f3A;
    a.y = o->f3E;
    a.z = o->f42;
    b.x = o->f6;
    b.y = o->fA;
    b.z = o->fE;
    c = b;
    p = D_80126B58;
    if (func_80135888(D_80126B78, D_80126B90, &a, &c) == 0) {
        return 0;
    }
    p += 4;
    t = func_80013478(p, &a);
    u = func_80013478(p, &b);
    if (t < u) {
        return 0;
    }
    ((s32 *)o)[1] = ((s32 *)o)[14];
    ((s32 *)o)[2] = ((s32 *)o)[15];
    ((s32 *)o)[3] = ((s32 *)o)[16];
    return 1;
}
