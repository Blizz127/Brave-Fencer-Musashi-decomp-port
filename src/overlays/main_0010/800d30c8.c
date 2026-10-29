/* Owner re-derivation from retail assembly (clean input set). */
/* Calls the handler selected by the byte at 0x15 of the object from the table D_800D3A8C, passing the object. Returns retail's leftover v0, the handler's result. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x15];
    u8 f15;
} Obj;

extern u32 D_800D3A8C[];

s32 func_800D30C8(Obj *o)
{
    return ((s32 (*)(Obj *))D_800D3A8C[o->f15])(o);
}
