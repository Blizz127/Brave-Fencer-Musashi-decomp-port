/* Owner re-derivation from retail assembly (clean input set). */
/* Converts the vector at v into two angles: v->x = -angle(v->y, hypot of x/z), v->y = angle(v->x, v->z), v->z = 0. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
} SV800D23D0;

extern s32 func_80047D3C(s32 v);
extern s32 func_8004CFEC(s32 y, s32 x);

void func_800D23D0(SV800D23D0 *v)
{
    s32 x;
    s32 y;
    s32 z;
    s32 r;

    x = v->x;
    z = v->z;
    y = v->y;
    r = func_80047D3C(x * x + z * z);
    v->x = -func_8004CFEC(y, r);
    v->y = func_8004CFEC(x, z);
    v->z = 0;
}
