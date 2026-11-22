/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern void func_80059A80(void *o, u32 a, u32 b);
void func_80146014(u32 *o) {
    if (o[3] != 0) {
        do {
            func_80059A80(o, o[2], o[3]);
            o += 4;
        } while (o[3] != 0);
    }
}
