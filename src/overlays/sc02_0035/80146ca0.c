/* Owner re-derivation from retail assembly (clean input set). */
/* Increments the 16-bit counter at +2. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x2];
    u16 cnt;
} Obj;

/* Returns retail's leftover v0: the incremented counter (untruncated). */
s32 func_80146CA0(Obj *o) {
    s32 r;
    r = o->cnt + 1;
    o->cnt = r;
    return r;
}
