/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

s32 func_801829A0(u8 *o)
{
    *(s16 *)(o + 0xFE) = 0;
#ifndef __mips__
    return 0;
#endif
}
