/* Owner re-derivation from retail assembly (clean input set). */
/* Bumps the global event counter and clears the event timer in the global state block. */
#include "psx_types.h"

extern u8 D_800AF630[];

void func_80011998(void) {
    register u8 *state = D_800AF630;

    (*(u16 *)(state + 0xA3BA))++;
    *(s16 *)(state + 0xA3BC) = 0;
}
