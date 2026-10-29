/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch: calls the handler selected by the 16-bit field at offset 2 of the object, from the member's handler table D_801AB158 (guest-address words). */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u16 sel;
} Obj;

extern u32 D_801AB158[];

void func_801848A8(Obj *o)
{
    ((void (*)(Obj *))D_801AB158[o->sel])(o);
}
