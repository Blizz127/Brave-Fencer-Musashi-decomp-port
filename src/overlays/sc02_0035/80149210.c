/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1E4];
    union {
        u32 w;
        u16 h[2];
    } u;
} Obj;

void func_80149210(Obj *o, s32 a) {
    o->u.w = a | (a << 16);
    o->u.h[0] = a;
    o->u.h[1] = a;
}
