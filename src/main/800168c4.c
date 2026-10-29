/* Owner re-derivation from retail assembly (clean input set). */
/* Adds a table step (by phase or current index) to a counter; reports whether it is full and a flag is set. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x188];
    u16 f188;
    u8 pad18A[0x19E - 0x18A];
    u16 f19E;
} State800AF630;

extern State800AF630 D_800AF630;
extern u16 D_800AF7CE; /* same storage as D_800AF630.f19E */
extern s32 D_800B9A18;
extern u8 D_80062BA0[];

s32 func_800168C4(s32 phase)
{
    State800AF630 *p = &D_800AF630;

    phase &= 3;
    if (phase == 0) {
        D_800AF7CE += D_80062BA0[D_800B9A18];
    } else {
        D_800AF7CE += D_80062BA0[phase];
    }
    if (p->f19E >= 0xFF) {
        if (p->f188 != 0) {
            return 1;
        }
    }
    return 0;
}
