/* Owner re-derivation from retail assembly (clean input set). */
/* Forwards the object, its word at offset 8 and the address of its block at offset 0x10 to the member's large update routine. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x8];
    u32 f8;
    u8 pad1[0x4];
    u8 f10[1];
} Obj;

extern void func_8017F5B4(Obj *o, u32 a, u8 *b);

void func_8017F590(Obj *o)
{
    func_8017F5B4(o, o->f8, o->f10);
}
