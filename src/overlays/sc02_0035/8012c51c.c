/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_8012C890(void *o, u8 *p, u8 *q);
extern s16 D_80126720;

s32 func_8012C51C(void *o, u8 *q) {
    u8 *end = ((u8 *)&D_80126720);
    u8 *p;
    if (q == 0) {
        p = end - 0x6480;
    } else {
        p = q + 0x10C;
    }
    if (p != end) {
        do {
            if (*(u16 *)p == 0) {
                goto found;
            }
            p += 0x10C;
        } while (p != end);
    }
    p = 0;
found:
    if (p == 0) {
        return 0;
    }
    return func_8012C890(o, p, q);
}
