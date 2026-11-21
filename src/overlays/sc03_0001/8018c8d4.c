/* Owner re-derivation from retail assembly (clean input set). */
/* Latches word `sel` of the current page's row of D_801C16C4 into D_801EF1E0. */
#include "psx_types.h"

extern s16 D_80115126;
extern u32 D_801C16C4[];
extern s32 D_801EF1E0;

void func_8018C8D4(s16 sel)
{
    D_801EF1E0 = D_801C16C4[D_80115126 * 10 + sel];
}
