/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch: calls the handler selected by the byte at offset 0x4 of the object, from the member's handler table D_801826A8 (guest-address words). Returns retail's leftover v0, the handler's result. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x4];
    u8 sel;
} Obj;

extern u32 D_801826A8[];

s32 func_8017D1B4(Obj *o)
{
    return ((s32 (*)(Obj *))D_801826A8[o->sel])(o);
}
