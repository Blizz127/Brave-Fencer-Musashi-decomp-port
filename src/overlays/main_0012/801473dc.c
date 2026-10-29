/* Owner re-derivation from retail assembly (clean input set). */
/* Zeroes three words at offsets 0x10..0x18 of the object. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x10];
    s32 a; /* 0x10 */
    s32 b; /* 0x14 */
    s32 c; /* 0x18 */
} Obj801473dc;

void func_801473DC(Obj801473dc *o)
{
    o->c = 0;
    o->b = 0;
    o->a = 0;
}
