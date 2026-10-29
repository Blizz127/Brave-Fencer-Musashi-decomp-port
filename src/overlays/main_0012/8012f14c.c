/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_8004914C(a0), func_800491AC(a0) and func_8004945C(a1, a2, &out) with a local 8-byte out buffer. */
#include "psx_types.h"

extern void func_8004914C(s32 a0);
extern void func_800491AC(s32 a0);
extern void func_8004945C(s32 a0, s32 a1, u16 *out);

void func_8012F14C(s32 a0, s32 a1, s32 a2)
{
    u16 out[4];

    func_8004914C(a0);
    func_800491AC(a0);
    func_8004945C(a1, a2, out);
}
