/* Owner re-derivation from retail assembly (clean input set). */
/* Polls func_800167F0(4); when its low 16 bits are non-zero calls func_800CFBCC(0) and returns 1, otherwise returns 0. */
#include "psx_types.h"

extern s32 func_800167F0(s32 a);
extern void func_800CFBCC(s32 a);

s32 func_8017CF9C(void)
{
    if ((u16)func_800167F0(4) == 0) {
        return 0;
    }
    func_800CFBCC(0);
    return 1;
}
