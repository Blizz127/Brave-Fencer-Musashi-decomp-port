/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch: calls the handler selected by the byte at offset 0x4 of the object, from the member's handler table D_80188DD0 (guest-address words). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x4];
    u8 sel;
} Obj;

extern u32 D_80188DD0[];

void func_8018133C(Obj *o)
{
    ((void (*)(Obj *))D_80188DD0[o->sel])(o);
}
