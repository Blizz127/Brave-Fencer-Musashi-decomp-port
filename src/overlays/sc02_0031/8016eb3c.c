/* Owner re-derivation from retail assembly (clean input set). */
/* func_8016EC0C(o, D_80188020[o->f2 & 3]); then a countdown: if o->f2 != 0, decrement it;
 * otherwise reset it to 5 and bump o->f1. No caller found in any available region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[1];
    u8 f1; /* 0x1 */
    u8 f2; /* 0x2 */
} Obj;

extern u8 D_80188020[];
extern void func_8016EC0C(Obj *o, u8 a1);

void func_8016EB3C(Obj *o)
{
    u8 old;

    func_8016EC0C(o, D_80188020[o->f2 & 3]);

    old = o->f2;
    o->f2 = old + 0xFF;
    if (old != 0) {
        return;
    }
    o->f2 = 5;
    o->f1 = o->f1 + 1;
}
