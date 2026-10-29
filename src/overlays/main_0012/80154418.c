/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_8015444C with three zeroed local out-slots. */
#include "psx_types.h"

extern void func_8015444C(void *a0, s32 *a1, s32 *a2, s32 *a3);

void func_80154418(void *a0)
{
    s32 a = 0;
    s32 b = 0;
    s32 c = 0;

    func_8015444C(a0, &a, &b, &c);
}
