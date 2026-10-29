/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern void func_80016714(u16 *p, s32 a);

void func_801292C8(char *o)
{
    u16 *p = *(u16 **)(o + 0x20);
    if (p) {
        s32 k = 0x38;
        if (*p == 1) {
            k = 0x84;
        }
        func_80016714(p, k);
    }
    *(u16 *)o = 0;
}
