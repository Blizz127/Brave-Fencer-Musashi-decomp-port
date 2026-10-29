/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x1C];
    s32 n;
} Obj80146E98;

s32 func_80146E98(Obj80146E98 *o) {
    s32 n = o->n;
    if (n != 0) {
        n--;
        o->n = n;
        return n == 0;
    }
    return 1;
}
