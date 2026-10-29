/* Owner re-derivation from retail assembly (clean input set). */
/* Resets the DMA channel registers for `mode`: writes 0x80000000 to the control register behind D_800DB670, clears the
 * channel words behind D_800DB644 and D_800DB650, then writes 0x60000000 to the control register. Mode 0 then submits
 * both 0x20-byte buffers through func_800D2E64; mode 1 reads the D_800DB650 register back once and discards the value;
 * any other mode prints the message D_800CEE1C with the mode through the printf-like func_8005C604 (retail keeps the
 * mode in a1 for that call, which is why the argument is passed). The D_800DB6xx globals hold guest register
 * addresses (u32). volatile is used only where retail codegen needs it: the final control write in both modes, the
 * D_800DB650 clear in mode 0 and the discarded read-back in mode 1 (otherwise the compiler fills the call/jump delay
 * slots with the store or the argument load that retail leaves in place). */
#include "psx_types.h"

extern u32 D_800DB670;
extern u32 D_800DB644;
extern u32 D_800DB650;
extern u32 D_800DB52C[];
extern u32 D_800DB5B0[];
extern u8 D_800CEE1C[];
extern void func_800D2E64(u32 *src, u32 len);
extern void func_8005C604(u8 *fmt, ...);

void func_800D2D68(s32 mode)
{
    switch (mode) {
    case 0:
        *(u32 *)D_800DB670 = 0x80000000;
        *(u32 *)D_800DB644 = 0;
        *(volatile u32 *)D_800DB650 = 0;
        *(volatile u32 *)D_800DB670 = 0x60000000;
        func_800D2E64(D_800DB52C, 0x20);
        func_800D2E64(D_800DB5B0, 0x20);
        break;
    case 1:
        *(u32 *)D_800DB670 = 0x80000000;
        *(u32 *)D_800DB644 = 0;
        *(u32 *)D_800DB650 = 0;
        *(volatile u32 *)D_800DB650;
        *(volatile u32 *)D_800DB670 = 0x60000000;
        break;
    default:
        func_8005C604(D_800CEE1C, mode);
        break;
    }
}
