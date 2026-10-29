/* Owner re-derivation from retail assembly (clean input set). */
/* Waits until bit 24 of the DMA control word behind D_800DB650 clears, counting a 0x100000 timeout down in the frame;
 * on timeout reports through func_800D30D0 and returns -1, otherwise returns 0. The timeout counter is volatile:
 * retail stores it and reloads it for every test. D_800DB650 holds a guest register address (u32). */
#include "psx_types.h"

extern u32 D_800DB650;
extern u8 D_800CEE48[];
extern void func_800D30D0(u8 *msg);

s32 func_800D3020(void)
{
    volatile s32 timeout = 0x100000;

    if ((*(u32 *)D_800DB650 & 0x01000000) != 0) {
        do {
            if (--timeout == -1) {
                func_800D30D0(D_800CEE48);
                return -1;
            }
        } while ((*(u32 *)D_800DB650 & 0x01000000) != 0);
    }
    return 0;
}
