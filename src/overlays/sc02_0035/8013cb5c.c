/* Owner re-derivation from retail assembly (clean input set). */
/* Copies two globals into two other globals. v0 is never touched; no callers found, so this keeps
 * the s32 ABI default rather than asserting void. */
#include "psx_types.h"

extern s32 D_801BD6D0;
extern s32 D_801BD6D4;
extern s32 D_801BD6A8;
extern s32 D_801BD6A4;

s32 func_8013CB5C(void)
{
    D_801BD6A8 = D_801BD6D0;
    D_801BD6A4 = D_801BD6D4;
#ifndef __mips__
    return 0;
#endif
}
