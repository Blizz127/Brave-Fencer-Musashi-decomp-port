/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    u8 pad0[4];
    Vec3 a;
    u8 padC[0x28];
    Vec3 b;
} Obj;

/* Returns retail's leftover v0 (x, the first word loaded from +0x38). */
s32 func_8012ADE4(Obj *o) {
    s32 x = o->b.x;
    s32 y = o->b.y;
    s32 z = o->b.z;
    o->a.x = x;
    o->a.y = y;
    o->a.z = z;
    return x;
}
