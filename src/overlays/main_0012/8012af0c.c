/* func_8012AF0C [8012AF0C,8012B014) - owner re-derivation from retail assembly (clean input set). */
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
    u8 p10[0x10];
    u32 f20;
    u8 p24[0x16];
    u16 f3A;
    u8 p3C[2];
    u16 f3E;
    u8 p40[2];
    u16 f42;
    u8 p44[0x14];
    u32 f58;
} O;

/* callers read v0: none-direct (no jal caller); returns 1/0.
 * o+4 and o+0x38 are 12-byte word vectors (copied word by word). func_800132BC takes two vectors. */
extern s32 func_80135888(u32 obj, u32 flags, void *from, void *to);
extern s32 func_800132BC(void *a, void *b);

s32 func_8012AF0C(O *o, O *q) {
    V3 a;
    V4 b;
    V4 c;
    V3 d;
    s32 t;
    s32 u;

    a.x = o->f3A;
    a.y = o->f3E;
    a.z = o->f42;
    b.x = o->f6;
    b.y = o->fA;
    b.z = o->fE;
    c = b;
    if (func_80135888(q->f20, q->f58, &a, &c) == 0) {
        return 0;
    }
    d.x = q->f6;
    d.y = q->fA;
    d.z = q->fE;
    t = func_800132BC(&d, &a);
    u = func_800132BC(&d, &b);
    if (t < u) {
        return 0;
    }
    ((s32 *)o)[1] = ((s32 *)o)[14];
    ((s32 *)o)[2] = ((s32 *)o)[15];
    ((s32 *)o)[3] = ((s32 *)o)[16];
    return 1;
}
