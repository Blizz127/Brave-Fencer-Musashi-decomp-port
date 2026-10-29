/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
s32 func_80146DE8(s32 *p, s32 a, s32 b, s32 c);
/* Returns retail's leftover v0: func_80146DE8's result. */
s32 func_80146DB8(s32 *p, s32 *q) {
    return func_80146DE8(p, q[0], q[1], q[2]);
}
