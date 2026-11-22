/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the low byte of func_80029178(0x80). */
#include "psx_types.h"

extern s32 func_80029178(u32 flag);

s32 func_8014BEF8(void *obj) /* obj unused; callers pass it (80155150: move a0,s0) */
{
    return (u8)func_80029178(0x80);
}
