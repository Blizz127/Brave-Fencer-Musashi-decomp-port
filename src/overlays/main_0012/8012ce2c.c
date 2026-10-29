/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 w;
} V4;

extern s32 func_80133784(s32 a, V4 *b, V4 *c);

s32 func_8012CE2C(u8 *o) {
    V4 v;
    V4 w;
    v.x = *(u16 *)(o + 6);
    v.y = *(u16 *)(o + 0xA);
    v.z = *(u16 *)(o + 0xE);
    w = v;
    w.y = w.y + 8;
    func_80133784(1, &v, &w);
    return v.y == w.y;
}
