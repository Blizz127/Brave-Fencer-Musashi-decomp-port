/* Owner re-derivation from retail assembly (clean input set). */
/* Setter: stores a 16-bit value into field +0x16C of the passed object. No caller found in any
 * available region/overlay; body never sets v0. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x16C];
    s16 f16C; /* 0x16C */
} Obj;

void func_8015115C(Obj *o, s16 v)
{
    o->f16C = v;
}
