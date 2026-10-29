/* Owner re-derivation from retail assembly (clean input set). */
/* Sets the 0x8000 flag bit in the halfword at offset 0x3C of the object. */
#include "psx_types.h"

void func_8014900C(u8 *obj) {
    *(u16 *)(obj + 0x3C) |= 0x8000;
}
