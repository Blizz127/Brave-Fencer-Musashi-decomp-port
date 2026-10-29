/* Owner re-derivation from retail assembly (clean input set). */
/* Starts a block DMA transfer from a buffer whose first word is the header: waits through func_800D2F88, ORs 0x88
 * into the control word, programs the address (buffer + 4), the block word ((len >> 5) blocks of 0x20 words), sends the
 * header word to the data port and starts the transfer. The D_800DB6xx globals hold guest register addresses (u32). */
#include "psx_types.h"

extern u32 D_800DB674;
extern u32 D_800DB63C;
extern u32 D_800DB640;
extern u32 D_800DB66C;
extern u32 D_800DB644;
extern void func_800D2F88(void);

void func_800D2E64(u32 *src, u32 len)
{
    func_800D2F88();
    *(u32 *)D_800DB674 |= 0x88;
    *(u32 *)D_800DB63C = (u32)(src + 1);
    *(u32 *)D_800DB640 = ((len >> 5) << 16) | 0x20;
    *(u32 *)D_800DB66C = *src;
    *(u32 *)D_800DB644 = 0x01000201;
}
