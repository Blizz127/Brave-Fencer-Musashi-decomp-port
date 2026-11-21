/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up the word at index D_80115126 * 10 + sel of the table D_801C16C4. */
#include "psx_types.h"

extern s16 D_80115126;
extern u32 D_801C16C4[];

u32 func_8018C89C(s16 sel)
{
    return D_801C16C4[D_80115126 * 10 + sel];
}
