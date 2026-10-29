/* Owner re-derivation from retail assembly (clean input set). */
/* Increments the 16-bit counter at +2 and clears the one at +0x34. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x2];
    u16 cnt;
    u8 pad4[0x30];
    u16 clr;
} Obj;

/* Returns retail's leftover v0: the incremented counter (untruncated). */
s32 func_8012AD50(Obj *o) {
    s32 r;
    r = o->cnt + 1;
    o->cnt = r;
    o->clr = 0;
    return r;
}
