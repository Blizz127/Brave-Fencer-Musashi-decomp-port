/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
s32 func_80146A6C(u16 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g);

/* Returns retail's leftover v0: func_80146A6C's result from the final call. */
s32 func_80146994(u16 a, s32 b, s32 c, s32 d) {
    return func_80146A6C(a, b, 0, 0, 0, c, d);
}
