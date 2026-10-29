/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch: calls the handler selected by the byte at offset 0x15 of the object, from the member's handler table D_8018267C (guest-address words). Returns retail's leftover v0, the handler's result. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x15];
    u8 sel;
} Obj;

extern u32 D_8018267C[];

s32 func_8017CF60(Obj *o)
{
    return ((s32 (*)(Obj *))D_8018267C[o->sel])(o);
}
