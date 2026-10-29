/* Owner re-derivation from retail assembly (clean input set). */
/* func_8012B030 - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 func_80134510(u16 *p);

s32 func_8012B030(u8 *o) {
    u16 t[3];
    t[0] = *(u16 *)(o + 6);
    t[1] = *(u16 *)(o + 0xA);
    t[2] = *(u16 *)(o + 0xE);
    if (func_80134510(t) == 0) {
        return 0;
    }
    *(u16 *)(o + 6) = t[0];
    *(u16 *)(o + 0xA) = t[1];
    *(u16 *)(o + 0xE) = t[2];
    *(u16 *)(o + 4) = *(u16 *)(o + 8) = *(u16 *)(o + 0xC) = 0;
    return 1;
}
