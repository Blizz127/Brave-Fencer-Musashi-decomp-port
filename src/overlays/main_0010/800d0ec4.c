/* Find the first free inventory slot in the resident script variables. */
#include "psx_types.h"

extern u8 func_800291B4(s32 index);

u32 func_800D0EC4(void)
{
    u32 slot = 0x2f;
    do {
        if (func_800291B4(slot) == 0)
            return slot;
        ++slot;
    } while (slot < 0x3b);
    return 0;
}
