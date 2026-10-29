/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u8 *func_8012913C(s32);

u8 *func_801290DC(u16 a, u16 *p) {
    u8 *r = func_8012913C(a);
    if (r != 0) {
        *(u16 *)(r + 6) = p[0];
        *(u16 *)(r + 0xA) = p[1];
        *(u16 *)(r + 0xE) = p[2];
    }
    return r;
}
