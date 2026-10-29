/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch: calls the handler selected by the byte at offset 0xC1 of the object, from the
 * table D_80184860, passing the object; returns the handler's v0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xC1];
    u8 sel;
} Obj;

extern u32 D_80184860[];

s32 func_80130D0C(Obj *o)
{
    return ((s32 (*)(Obj *))D_80184860[o->sel])(o);
}
