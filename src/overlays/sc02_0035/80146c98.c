/* Owner re-derivation from retail assembly (clean input set). */
/* Setter: stores a 16-bit value into field +0x2 of the passed object. */
#include "psx_types.h"

typedef struct {
    u8 pad00[2];
    s16 f2; /* 0x2 */
} Obj;

void func_80146C98(Obj *o, s16 v)
{
    o->f2 = v;
}
