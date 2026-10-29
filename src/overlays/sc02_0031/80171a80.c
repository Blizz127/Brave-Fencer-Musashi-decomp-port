/* Owner re-derivation from retail assembly (clean input set). */
/* Setter: stores a 32-bit value into field +0x200 of the passed object. No caller found in any
 * available region/overlay; body never sets v0. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x200];
    s32 f200; /* 0x200 */
} Obj;

void func_80171A80(Obj *o, s32 v)
{
    o->f200 = v;
}
