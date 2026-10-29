/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 0 if flags 0x404 are set in the word at 0x44, otherwise whether bit 0x80 is set in the halfword at 0xAC. */
#include "psx_types.h"

s32 func_80149AA8(u8 *obj) {
    u32 t;
    if ((*(u32 *)(obj + 0x44) & 0x404) == 0) {
        t = *(u16 *)(obj + 0xAC) & 0x80;
        return t != 0;
    }
    return 0;
}
