/* Owner re-derivation from retail assembly (clean input set). */
/* Starts a block DMA transfer: waits through func_800D3020, ORs 0x88 into the control word, then programs the
 * address (a0), the block word ((a1 >> 5) blocks of 0x20 words) and starts the transfer. The D_800DB6xx globals hold
 * guest register addresses (u32). */
#include "psx_types.h"

extern u32 D_800DB674;
extern u32 D_800DB650;
extern u32 D_800DB648;
extern u32 D_800DB64C;
extern void func_800D3020(void);

void func_800D2EF8(u32 a0, u32 a1)
{
    func_800D3020();
    *(u32 *)D_800DB674 |= 0x88;
    *(u32 *)D_800DB650 = 0;
    *(u32 *)D_800DB648 = a0;
    *(u32 *)D_800DB64C = ((a1 >> 5) << 16) | 0x20;
    *(u32 *)D_800DB650 = 0x01000200;
}
