/* Owner re-derivation from retail assembly (clean input set). */
/* Sets the global event counter to a given value and clears the event timer. */
#include "psx_types.h"

extern u8 D_800AF630[];

void func_800119F0(u16 value) {
    register u8 *state = D_800AF630;

    *(u16 *)(state + 0xA3BA) = value;
    *(s16 *)(state + 0xA3BC) = 0;
}
