/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[8];
    s32 f8;
    u8 pad0C[0x28];
    s32 f34;
} Obj;

/* Returns retail's leftover v0: the new f8 (f8 + f34). */
s32 func_80147460(Obj *o) {
    s32 r;
    r = o->f8 + o->f34;
    o->f8 = r;
    return r;
}
