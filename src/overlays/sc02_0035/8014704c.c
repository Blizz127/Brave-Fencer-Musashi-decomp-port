/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

s32 func_8014704C(u8 *o)
{
    *(u8 *)(o + 0x4C) = 0;
#ifndef __mips__
    return 0;
#endif
}
