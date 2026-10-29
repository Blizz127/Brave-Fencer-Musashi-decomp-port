/* Owner re-derivation from retail assembly (clean input set). */
/* Clears bit 14 of the flags halfword at +0x3C. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x3C];
    u16 flags;
} Obj;

void func_80149048(Obj *o) {
    o->flags &= 0xBFFF;
}
