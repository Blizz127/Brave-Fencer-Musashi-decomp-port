#include "psx_types.h"

s32 func_8012BEE8(u8 *actor)
{
    u32 timer = *(u32 *)(actor + 0x1c);
    if (timer != 0) {
        *(u32 *)(actor + 0x1c) = --timer;
        return timer == 0;
    }
    return 1;
}
