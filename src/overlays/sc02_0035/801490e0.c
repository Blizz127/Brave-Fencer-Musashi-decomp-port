/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

s32 func_801490E0(u8 *o, s16 a1)
{
    *(s16 *)(o + 0x108) = a1;
#ifndef __mips__
    return 0;
#endif
}
