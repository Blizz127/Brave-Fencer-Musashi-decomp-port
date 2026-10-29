/* Owner re-derivation from retail assembly (clean input set). */
/* Probes func_8012DEB8 with two short vectors built from the box at a1 (x from +4, then from +6; z from +0xC and +0xE); returns 1 on the first hit, else whether the second probe hit. */
#include "psx_types.h"

typedef struct {
    s16 x, y, z;
} Vec3s;

typedef struct {
    u8 pad0[4];
    u16 f4;
    u16 f6;
    u8 pad8[4];
    u16 fC;
    u16 fE;
} Box;

extern s32 func_8012DEB8(void *o, Vec3s *a, Vec3s *b);

s32 func_801818A4(void *o, Box *b)
{
    Vec3s v1;
    Vec3s v2;
    s32 r;

    v1.z = b->fC;
    v2.z = b->fE;
    v1.y = v2.y = 0;
    v1.x = v2.x = b->f4;
    if (func_8012DEB8(o, &v1, &v2) != 0) {
        r = 1;
    } else {
        v1.x = v2.x = b->f6;
        r = func_8012DEB8(o, &v1, &v2) != 0;
    }
    return r;
}
