/* Owner re-derivation from retail assembly (clean input set). */
/* Stores the result of func_8004239C(1) in the current buffer's slot of D_800AE810. */
#include "psx_types.h"

extern s32 func_8004239C(s32);
extern s32 D_800AE810[];
extern u8 D_800AF630[];

void func_8001099C(void) {
    register u8 *state = D_800AF630;

    D_800AE810[*(u16 *)(state + 0xA3D2)] = func_8004239C(1);
}
