/* Owner re-derivation from retail assembly (clean input set). */
/* If flag 0x10 is set in o->flags, lowers the s16 at 0x188 by 4, clamping at 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 flags; /* 0x44 */
    u8 pad1[0x188 - 0x48];
    s16 v; /* 0x188 */
} Obj8014a638;

void func_8014A638(Obj8014a638 *o)
{
    if (o->flags & 0x10) {
        o->v -= 4;
        if (o->v < 0) {
            o->v = 0;
        }
    }
}
