/* Owner re-derivation from retail assembly (clean input set). */
/* Calls the handler selected by the byte at 0x214 of the object from the table D_800D3A98, passing the object. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x214];
    u8 kind;
} Obj;

extern u32 D_800D3A98[];

void func_800D3238(Obj *o)
{
    ((void (*)(Obj *))D_800D3A98[o->kind])(o);
}
