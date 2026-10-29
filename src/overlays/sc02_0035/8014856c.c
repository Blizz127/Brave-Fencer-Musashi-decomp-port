/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u8 D_80126B58[];
extern s32 func_80154358(u8 *);
extern void func_801485B8(s32, s32, s32);

void func_8014856C(s32 a, s32 b) {
    func_801485B8(func_80154358(D_80126B58) - 0x8, a, b);
}
