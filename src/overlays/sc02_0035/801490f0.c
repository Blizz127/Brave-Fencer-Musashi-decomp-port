/* Owner re-derivation from retail assembly (clean input set). */
/* Setter: stores a 16-bit value into field +0x10C of the passed object. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x10C];
    s16 f10C; /* 0x10C */
} Obj;

void func_801490F0(Obj *o, s16 v)
{
    o->f10C = v;
}
