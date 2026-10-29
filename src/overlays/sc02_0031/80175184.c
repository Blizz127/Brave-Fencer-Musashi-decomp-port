/* Owner re-derivation from retail assembly (clean input set). */
/* If o->2 (u16) < 2, dispatches through D_801882F8[idx] (no args set, void(void));
 * else calls func_80175268(void). */
#include "psx_types.h"

extern void (*D_801882F8[])(void);
extern void func_80175268(void);

void func_80175184(u8 *a0)
{
    u16 idx = *(u16 *)(a0 + 2);
    if (idx < 2) {
        D_801882F8[idx]();
    } else {
        func_80175268();
    }
}
