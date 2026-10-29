/* Owner re-derivation from retail assembly (clean input set). */
/* Clears byte at offset 0x1C7 of the object. */
#include "psx_types.h"

typedef struct {
    u8 pad[455];
    u8 f; /* 0x1C7 */
} Obj8014b5d0;

void func_8014B5D0(Obj8014b5d0 *o)
{
    o->f = 0;
}
