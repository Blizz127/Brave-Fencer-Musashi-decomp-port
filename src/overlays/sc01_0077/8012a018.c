/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern u8 D_80126948[];
s32 func_8012A048(u8 *p, u32 callback_address, u8 mode);

/* Returns retail's leftover v0: func_8012A048's result. */
s32 func_8012A018(u32 callback_address, s32 mode) {
    return func_8012A048(D_80126948, callback_address, (u8)mode);
}
