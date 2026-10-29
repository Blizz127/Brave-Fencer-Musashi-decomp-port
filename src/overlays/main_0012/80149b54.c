/* Owner re-derivation from retail assembly (clean input set). */
/* True when flag 0x400 is clear, func_8014BEF8 accepts the object, and bit 0x10 of its 0xAC halfword is set. */
#include "psx_types.h"

extern s32 func_8014BEF8(u8 *obj);

s32 func_80149B54(u8 *obj)
{
    if (*(u32 *)(obj + 0x44) & 0x400) {
        return 0;
    }
    if (func_8014BEF8(obj) == 0) {
        return 0;
    }
    if (*(u16 *)(obj + 0xAC) & 0x10) {
        return 1;
    }
    return 0;
}
