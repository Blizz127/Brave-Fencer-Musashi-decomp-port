/* Owner re-derivation from retail assembly (clean input set). */
/* Sets the secondary event counter to a given value and clears its timer. */
#include "psx_types.h"

extern u8 D_800AF630[];

void func_80011D54(u16 value) {
    register u8 *state = D_800AF630;

    *(u16 *)(state + 0xA3CC) = value;
    *(s16 *)(state + 0xA3CE) = 0;
}
