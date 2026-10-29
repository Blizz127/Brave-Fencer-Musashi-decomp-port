/* Owner re-derivation from retail assembly (clean input set). */
/* Resets four state globals; retail falls off the end with v0 = 1 from the last store. */
#include "psx_types.h"

extern s32 D_801BD6A4;
extern s32 D_801BD698;
extern s32 D_801BD69C;
extern s32 D_801BD6A8;

s32 func_8013CABC(void)
{
    D_801BD6A4 = 1;
    D_801BD698 = 1;
    D_801BD69C = 0;
    D_801BD6A8 = 1;
#ifndef __mips__
    return 0;
#endif
}
