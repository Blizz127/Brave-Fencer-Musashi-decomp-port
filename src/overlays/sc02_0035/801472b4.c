/* Owner re-derivation from retail assembly (clean input set). */
/* Clears bit 1 of the flags halfword at +0x7E. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x7E];
    u16 flags;
} Obj;

/* Returns retail's leftover v0 (the new flags value). */
s32 func_801472B4(Obj *o) {
    s32 r = o->flags & 0xFFFD;
    o->flags = r;
    return r;
}
