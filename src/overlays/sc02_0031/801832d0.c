/* Owner re-derivation from retail assembly (clean input set). */
/* Sets the current base word to a + 0x380, clears the counter word and records b. */
#include "psx_types.h"

extern u32 D_801B794C;
extern u32 D_801B7930;
extern u32 D_801B7988;

void func_801832D0(u32 a, u32 b)
{
    D_801B794C = a + 0x380;
    D_801B7930 = 0;
    D_801B7988 = b;
}
