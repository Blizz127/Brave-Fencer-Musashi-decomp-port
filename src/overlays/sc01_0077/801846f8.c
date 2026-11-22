/* Hold the fifth state for twenty game updates. */
#include "psx_types.h"

void func_801846F8(u8 *actor)
{
    *(u16 *)(actor + 2) = 5;
    *(s32 *)(actor + 0x1c) = 20;
}
