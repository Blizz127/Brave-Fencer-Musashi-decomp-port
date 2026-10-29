/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_800425E0 with the constant 1 and the argument; nothing read back. */
#include "psx_types.h"

extern s32 func_800425E0(s32 a, s32 b);

void func_800D2D44(s32 a0)
{
    func_800425E0(1, a0);
}
