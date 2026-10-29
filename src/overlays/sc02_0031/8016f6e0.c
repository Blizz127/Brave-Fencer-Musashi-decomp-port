/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch: calls the handler selected by the byte at offset 0x216 of the object, from the
 * table D_801880C4 (guest-address words), passing the object as a0; returns the handler's v0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x216];
    u8 sel;
} Obj;

extern u32 D_801880C4[];

s32 func_8016F6E0(Obj *o)
{
    return ((s32 (*)(Obj *))D_801880C4[o->sel])(o);
}
