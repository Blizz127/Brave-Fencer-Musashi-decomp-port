/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the first voice that is idle (state 2) and not held, or -1 if none. */
/* Verified 22/22 words with the pinned oracle, gcc-2.7.2-psx / ASPSX 2.56, -O2 -G0. */
#include "psx_types.h"

extern u8 *D_800762B0;
extern u8 D_800762B4[];

s32 func_80038A00(void) {
    u8 *state;
    u8 *hold;
    s32 i;

    state = D_800762B0;
    hold = D_800762B4;
    for (i = 0; i < 16; i++, state++, hold++) {
        if (*state == 2 && *hold == 0) {
            return i;
        }
    }
    return -1;
}
