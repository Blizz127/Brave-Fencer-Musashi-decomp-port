/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Sets flag 0x8 and runs the step handler D_800CBB24[o->step]. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 flags;          /* 0x44 */
    u8 pad48[0x197 - 0x48];
    u8 step;            /* 0x197 */
} Obj;

extern u32 D_800CBB24[];

void func_800CAE34(Obj *o)
{
    o->flags |= 8;
    ((void (*)(Obj *))D_800CBB24[o->step])(o);
}
