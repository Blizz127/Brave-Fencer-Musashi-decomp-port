/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_8002D4C8(2, 0) and sets a halfword in the game state block to 1. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern void func_8002D4C8(s32, s32);

void func_80010E7C(void) {
    register u8 *base = D_800AF630;

    func_8002D4C8(2, 0);
    *(s16 *)(base + 0xA3D4) = 1;
}
