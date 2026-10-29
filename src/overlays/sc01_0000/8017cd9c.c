/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch: calls the handler selected by the byte at offset 0x15 of the object, from the member's handler table D_80182668 (guest-address words). Returns retail's leftover v0, the handler's result. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x15];
    u8 sel;
} Obj;

extern u32 D_80182668[];

s32 func_8017CD9C(Obj *o)
{
    return ((s32 (*)(Obj *))D_80182668[o->sel])(o);
}
