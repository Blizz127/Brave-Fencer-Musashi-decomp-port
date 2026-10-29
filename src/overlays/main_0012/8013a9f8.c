/* Owner re-derivation from retail assembly (clean input set). */
/* For a in [2,6] stores (a-2)<<10 at +0xE. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xE];
    s16 fE;
} Obj8013A9F8;

void func_8013A9F8(Obj8013A9F8 *o, s32 a) {
    u16 b = a;
    s32 v;
    if ((u16)(a - 2) < 5) {
        v = (b - 2) << 10;
        o->fE = v;
    }
}
