/* Owner re-derivation from retail assembly (clean input set). */
/* Clamps bytes 2..7 of the 0x32-offset array: 0x51..0xAF become 0x80, below 8 become 0, 0xF8 and above become 0xFF. */
#include "psx_types.h"

void func_80018F20(u8 *base) {
    u32 i;

    for (i = 2; i < 8; i++) {
        u8 *p = base + i;
        u8 c = p[0x32];

        if (c >= 0x51 && c < 0xB0) {
            p[0x32] = 0x80;
        } else if (c < 8) {
            p[0x32] = 0;
        } else if (c >= 0xF8) {
            p[0x32] = 0xFF;
        }
    }
}
