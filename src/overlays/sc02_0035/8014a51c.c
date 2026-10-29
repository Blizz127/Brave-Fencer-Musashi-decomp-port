/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u8 func_80029178(s32 a);

int func_8014A51C(u8 *o) {
    u32 t;
    if (func_80029178(0x1E) == 0) return 0;
    if (*(u32 *)(o + 0x44) & 0x400) return 0;
    if (*(u8 *)(o + 0xA8) >= 0x19) return 0;
    if (*(u8 *)(o + 0x223) != 0) return 0;
    t = *(u16 *)(o + 0xAC) & 0x40;
    if (t != 0) return 1;
    return 0;
}
