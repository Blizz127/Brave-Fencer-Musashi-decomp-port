/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern void *func_8001CFDC(s32 first, s32 limit);

void *func_8012C1B8(void) {
    return func_8001CFDC(0x26, 0x85);
}
