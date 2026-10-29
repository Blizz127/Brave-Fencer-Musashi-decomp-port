/* Owner re-derivation from retail assembly (clean input set). */
/* Clears byte at offset 0x1C4 of the object. */
#include "psx_types.h"

typedef struct {
    u8 pad[452];
    u8 f; /* 0x1C4 */
} Obj8014b5b0;

void func_8014B5B0(Obj8014b5b0 *o)
{
    o->f = 0;
}
