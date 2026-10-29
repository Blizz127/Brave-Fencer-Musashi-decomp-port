/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_8004CFEC(s32 a, s32 b);

/* Heading from a to b: (atan2(a.z - b.z, b.x - a.x) - 0x400) & 0xFFF. */
s32 func_8012B70C(s16 *a, s16 *b)
{
    return (func_8004CFEC(a[2] - b[2], b[0] - a[0]) - 0x400) & 0xFFF;
}
