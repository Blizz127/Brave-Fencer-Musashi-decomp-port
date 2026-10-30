/* Owner re-derivation from retail assembly (clean room, clean input set).
 * Moves voice bits between the masks D_800A2BA0 and D_800C7D2C: mode 1 clears them in
 * D_800A2BA0 and sets them in D_800C7D2C, any other mode does the reverse. Same shape as
 * the owner's func_8002EFF8 on the neighbouring mask pair. Callers ignore v0. */
#include "psx_types.h"

extern u32 D_800A2BA0;
extern u32 D_800C7D2C;

void func_8002F064(s32 mode, u32 bits) {
    if (mode == 1) {
        D_800A2BA0 &= ~bits;
        D_800C7D2C |= bits;
    } else {
        D_800C7D2C &= ~bits;
        D_800A2BA0 |= bits;
    }
}
