/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch: calls the handler selected by the 16-bit field at offset 2 of the object, from the table D_80185E74 (guest-address words), passing the object as a0; returns the handler's v0. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u16 sel;
} Obj;

extern u32 D_80185E74[];

s32 func_801449C8(Obj *o)
{
    return ((s32 (*)(Obj *))D_80185E74[o->sel])(o);
}
