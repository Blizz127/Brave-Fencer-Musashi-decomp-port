/* Owner re-derivation from retail assembly (clean input set). */
/* Ends a master fade-out: clears the fading-out flag and, unless locked, starts a fade back up at the speed for the given type. */
#include "psx_types.h"

extern u16 D_8006A9E0[];
extern u16 D_800A4EE2;
extern s16 D_800A4EE4;
extern u8 D_800A4EE6[];

void func_8002E8DC(u16 type) {
    u8 flags;

    flags = D_800A4EE6[0];
    if (flags & 2) {
        D_800A4EE6[0] = flags & 0xFD;
        if (!(flags & 4)) {
            if (type >= 5) {
                type = 4;
            }
            D_800A4EE4 = 0x4000;
            D_800A4EE2 = D_8006A9E0[type];
            D_800A4EE6[0] = (flags & 0xFD) | 1;
        }
    }
}
