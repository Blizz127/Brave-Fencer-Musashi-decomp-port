/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

void func_801479BC(char *o, s32 a, s32 lim)
{
    *(s32 *)(o + 0x28) += a;
    if (a < 0) {
        if (*(s32 *)(o + 0x28) <= -lim) {
            *(s32 *)(o + 0x28) = -lim;
            return;
        }
    }
    if (a > 0) {
        if (*(s32 *)(o + 0x28) >= lim) {
            *(s32 *)(o + 0x28) = lim;
        }
    }
}
