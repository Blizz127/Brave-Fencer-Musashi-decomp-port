/* Owner re-derivation from retail assembly (clean input set). */
/* Unless o->f44 bit 0x2 is set, runs func_80171A1C(o); func_80174650(o) [callee ignores o]. No
 * caller found in any available region/overlay; body never sets v0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 f44; /* 0x44 */
} Obj;

extern void func_80171A1C(void *o);
extern void func_80174650();

void func_801739D8(Obj *o)
{
    if ((o->f44 & 2) == 0) {
        func_80171A1C(o);
        func_80174650(o);
    }
}
