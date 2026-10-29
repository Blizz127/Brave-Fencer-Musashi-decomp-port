/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern u8 D_80126948[];
s32 func_8012A048(void *p, s32 a, s32 b);

/* Returns retail's leftover v0: func_8012A048's result. */
s32 func_8012A018(s32 a, s32 b) {
    return func_8012A048(((u8 *)&D_80126948), a, b & 0xFF);
}
