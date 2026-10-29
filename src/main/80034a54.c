/* Owner re-derivation from retail assembly (clean input set). */
/* If bit 0 of a flag byte is set, force a mode byte and reset two range halfwords. */
#include "psx_types.h"

extern u8 D_800A4F17;

void func_80034A54(u8 *arg) {
    u8 *obj = arg;

    if (obj[0x37] & 1) {
        u8 *flag = &D_800A4F17;
        u8 saved;

        saved = *flag;
        *flag = 1;
        obj[0x32] = 1;
        *(s16 *)(obj + 0x2E) = 0x3FF;
        *(s16 *)(obj + 0x30) = 0xFFF;
        *flag = saved;
    }
}
