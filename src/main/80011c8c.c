/* Owner re-derivation from retail assembly (clean input set). */
/* Sets the secondary event id and resets its counters and timers in the global state block. */
#include "psx_types.h"

extern u8 D_800AF630[];

void func_80011C8C(u16 id) {
    register u8 *state = D_800AF630;

    *(u16 *)(state + 0xA3C6) = id;
    *(s16 *)(state + 0xA3CC) = 0;
    *(s16 *)(state + 0xA3D0) = 0;
    *(s16 *)(state + 0xA3C8) = 0;
    *(s16 *)(state + 0xA3CE) = 0;
}
