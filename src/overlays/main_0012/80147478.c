/* Owner re-derivation from retail assembly (clean input set). */
/* One-shot init: if flag 0x40000000 is clear, runs the two setup calls and sets it. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 flags; /* 0x44 */
} Obj80147478;

extern void func_80147514(Obj80147478 *o);
extern void func_80147628(Obj80147478 *o);

void func_80147478(Obj80147478 *o)
{
    if (!(o->flags & 0x40000000)) {
        func_80147514(o);
        func_80147628(o);
        o->flags |= 0x40000000;
    }
}
