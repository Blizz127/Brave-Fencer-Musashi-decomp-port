/* Owner re-derivation from retail assembly (clean input set). */
/* Sets the current event id and resets the event counters and timers in the global state block. */
#include "psx_types.h"

extern u8 D_800AF630[];

void func_80011928(u16 id) {
    register u8 *state = D_800AF630;

    *(u16 *)(state + 0xA3B4) = id;
    *(s16 *)(state + 0xA3BA) = 0;
    *(s16 *)(state + 0xA3BE) = 0;
    *(s16 *)(state + 0xA3B6) = 0;
    *(s16 *)(state + 0xA3BC) = 0;
}
