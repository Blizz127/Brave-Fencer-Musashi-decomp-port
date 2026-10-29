/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

s32 func_80156A14(u8 *o)
{
    *(u8 *)(o + 0xDA) = 0;
#ifndef __mips__
    return 0;
#endif
}
