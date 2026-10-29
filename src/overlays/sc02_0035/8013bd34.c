/* Owner re-derivation from retail assembly (clean input set). */
/* Forwards its argument to func_8013BD74 together with &D_801BE9D8. func_8013BD74 is itself void;
 * no callers of func_8013BD34 found, so this keeps the s32 ABI default rather than asserting void. */
#include "psx_types.h"

extern u8 D_801BE9D8;
extern void func_8013BD74(void *a0, void *a1);

s32 func_8013BD34(void *a1)
{
    func_8013BD74(&D_801BE9D8, a1);
#ifndef __mips__
    return 0;
#endif
}
