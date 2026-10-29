/* Owner re-derivation from retail assembly (clean input set). */
/* Starts a master fade-out of the given type unless one is already running or the fade is locked. */
#include "psx_types.h"

extern u16 D_8006A9C8[];
extern s16 D_800A4EE0;
extern u16 D_800A4EE2;
extern s16 D_800A4EE4;
extern u8 D_800A4EE6[];

void func_8002E79C(u16 type) {
    u8 flags;
    u8 updated;

    flags = D_800A4EE6[0];
    if (!(flags & 2)) {
        updated = flags | 2;
        D_800A4EE6[0] = updated;
        if (!(updated & 4)) {
            if (type >= 5) {
                type = 4;
            }
            D_800A4EE0 = 0x4000;
            D_800A4EE4 = 0x2FFF;
            D_800A4EE2 = D_8006A9C8[type];
            D_800A4EE6[0] = flags | 3;
        }
    }
}
