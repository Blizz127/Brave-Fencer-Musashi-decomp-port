/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    u16 h6;
    u8 pad8[2];
    u16 hA;
    u8 padC[2];
    u16 hE;
    u8 pad10[0x164];
    s32 f174;
} O;

typedef struct {
    s16 x, y, z, w;
} R;

extern s32 func_8014CF04(O *o, R *a, R *b);
extern s32 func_8014CD80(O *o, R *a, R *b);

s32 func_8014CD0C(O *o) {
    R a;
    R b;
    s32 y;
    s32 r;
    a.x = b.x = o->h6;
    a.y = (y = o->hA) - 3;
    a.z = b.z = o->hE;
    b.y = y + 5;
    if (o->f174 == 0) {
        r = func_8014CF04(o, &a, &b);
    } else {
        r = func_8014CD80(o, &a, &b);
    }
    return r;
}
