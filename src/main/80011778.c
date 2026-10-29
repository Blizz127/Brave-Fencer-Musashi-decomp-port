/* Owner re-derivation from retail assembly (clean input set). */
/* Advances the main event counter and resets all event ids, counters and timers in the global state block. */
#include "psx_types.h"

extern u8 D_800AF630[];

void func_80011778(void) {
    register u8 *state = D_800AF630;

    (*(u16 *)(state + 0xA3AE))++;
    *(s16 *)(state + 0xA3B4) = 0;
    *(s16 *)(state + 0xA3BA) = 0;
    *(s16 *)(state + 0xA3B8) = 0;
    *(s16 *)(state + 0xA3BE) = 0;
    *(s16 *)(state + 0xA3B0) = 0;
    *(s16 *)(state + 0xA3B6) = 0;
    *(s16 *)(state + 0xA3BC) = 0;
}
