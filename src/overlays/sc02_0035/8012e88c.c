/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    u32 flags;
} Inner;

typedef struct {
    u8 pad0[0x20];
    Inner *in;
} Obj;

void func_8012E88C(Obj *o) {
    o->in->flags &= 0x7FFFFFFF;
}
