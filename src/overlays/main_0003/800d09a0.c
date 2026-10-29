/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame tick: bumps a counter, forwards a toggle bit to a handler, and calls a reset routine past 32. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern u16 D_800EC898;
extern s16 D_800EC894;
extern void func_800D12D0(void);
extern void func_800D1D14(s32, s32);
extern void func_800118AC(void);

void func_800D09A0(void) {
    register u8 *unused = D_800AF630;
    func_800D12D0();
    D_800EC898++;
    if (((s16)D_800EC898 & 4) != 0) {
        func_800D1D14(D_800EC894, 1);
    } else {
        func_800D1D14(D_800EC894, 0);
    }
    if ((s16)D_800EC898 > 0x20) {
        func_800118AC();
    }
}
