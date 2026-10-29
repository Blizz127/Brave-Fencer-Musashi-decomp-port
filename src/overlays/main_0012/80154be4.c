/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_80154C24 with two zeroed out-slots, then func_80154F9C. */
#include "psx_types.h"

extern void func_80154C24(void *a0, s32 *a1, s32 *a2);
extern void func_80154F9C(void *a0);

void func_80154BE4(void *a0)
{
    s32 a = 0;
    s32 b = 0;

    func_80154C24(a0, &a, &b);
    func_80154F9C(a0);
}
