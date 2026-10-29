/* Owner re-derivation from retail assembly (clean input set). */
/* Stores a1 to the halfword at offset 0x34 of the object. v0 is never touched; no callers found,
 * so this keeps the s32 ABI default rather than asserting void. */
#include "psx_types.h"

s32 func_8012AD64(u8 *o, s16 a1)
{
    *(s16 *)(o + 0x34) = a1;
#ifndef __mips__
    return 0;
#endif
}
