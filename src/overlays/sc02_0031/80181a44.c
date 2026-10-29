/* Owner re-derivation from retail assembly (clean input set). */
/* When the low two bits of D_800B99DA are clear: picks sound id 0x537 or 0x538 by flag 0xCE, queries func_80181AE0 on D_80188DF0 and, unless it returns 0xFFFF, starts the sound with a volume derived from the answer. */
#include "psx_types.h"

extern u16 D_800B99DA;
extern u8 D_80188DF0[];
extern u8 func_800291B4(s32 id);
extern u16 func_80181AE0(void *p, s32 n);
extern void func_8002D4C8(s32 id, u16 vol);

void func_80181A44(void)
{
    s32 id;
    s32 x;

    if ((D_800B99DA & 3) == 0) {
        id = func_800291B4(0xCE) != 0 ? 0x537 : 0x538;
        x = func_80181AE0(D_80188DF0, 0x1500);
        if (x != 0xFFFF) {
            func_8002D4C8(id, (0x7F - x / 56) | 0x1000);
        }
    }
}
