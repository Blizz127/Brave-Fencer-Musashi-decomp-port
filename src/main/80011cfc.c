/* Owner re-derivation from retail assembly (clean input set). */
/* Bumps the secondary event counter and clears its timer in the global state block. */
#include "psx_types.h"

extern u8 D_800AF630[];

void func_80011CFC(void) {
    register u8 *state = D_800AF630;

    (*(u16 *)(state + 0xA3CC))++;
    *(s16 *)(state + 0xA3CE) = 0;
}
