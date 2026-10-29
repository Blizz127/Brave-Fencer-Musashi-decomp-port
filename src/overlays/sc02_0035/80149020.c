/* Owner re-derivation from retail assembly (clean input set). */
/* Clears bit 15 of the flags halfword at +0x3C. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x3C];
    u16 flags;
} Obj;

/* Returns retail's leftover v0 (the new flags value). */
s32 func_80149020(Obj *o) {
    s32 r = o->flags & 0x7FFF;
    o->flags = r;
    return r;
}
