/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the 0x80 bit of the flags word at +0x44. */
#include "psx_types.h"

typedef struct {
    u8 pad000[0x44];
    u32 flags44;
} Obj8014B154;

u32 func_8014B154(Obj8014B154 *obj)
{
    return obj->flags44 & 0x80;
}
