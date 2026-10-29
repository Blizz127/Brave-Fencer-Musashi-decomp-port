/* Owner re-derivation from retail assembly (clean input set). */
/* State dispatch through the member's handler table D_80188FB8 (guest-address words) on the 16-bit field at offset 2; then, while the object's field 0 is non-zero, re-anchors the shared base from fields 0xA and 0xFE. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u16 sel;
    u8 pad0[0xA - 0x4];
    s16 fA;
    u8 pad1[0xFE - 0xC];
    s16 fFE;
} Obj;

extern u32 D_80188FB8[];
extern void func_801832D0(u32 a, u32 b);

void func_80182078(Obj *o)
{
    ((void (*)(Obj *))D_80188FB8[o->sel])(o);
    if (o->f0 != 0) {
        func_801832D0(o->fA - 0x380, o->fFE);
    }
}
