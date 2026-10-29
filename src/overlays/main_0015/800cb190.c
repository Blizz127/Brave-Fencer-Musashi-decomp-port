/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Clears flag 0x2 and applies effect 0x11. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 flags;          /* 0x44 */
} Obj;

extern void func_80154A74(void *o, s32 i);

void func_800CB190(Obj *o)
{
    o->flags &= ~2;
    func_80154A74(o, 0x11);
}
