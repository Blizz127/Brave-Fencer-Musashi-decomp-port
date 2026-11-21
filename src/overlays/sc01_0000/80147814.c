/* Fresh isolated derivation from sc01_0000 retail instructions. */
#include "psx_types.h"

void func_80147814(u8 *object, s32 delta)
{
    s32 sum = (s32)(*(u32 *)(object + 0x34) + (u32)delta);
    *(u32 *)(object + 0x34) = (u32)sum;
    if (delta < 0 && sum < -0x1F0000) {
        *(u32 *)(object + 0x34) = (u32)-0x1F0000;
        return;
    }
    if (delta > 0 && *(s32 *)(object + 0x34) > 0x1F0000) {
        *(u32 *)(object + 0x34) = 0x1F0000;
    }
}
