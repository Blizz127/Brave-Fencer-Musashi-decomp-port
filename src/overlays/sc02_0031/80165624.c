/* Owner re-derivation from retail assembly (clean input set). */
/* Compares the masked return of func_80165658(o, D_80078EC1 & 0x7F) against D_80078EB8 (both
 * masked/widened the same way) and returns whether the table value is strictly below it. The
 * caller-supplied object pointer is forwarded to func_80165658 unexamined otherwise. */
#include "psx_types.h"

extern u8 D_80078EC1;
extern u16 D_80078EB8;
extern u8 func_80165658(s32 o, u32 idx);

s32 func_80165624(s32 o)
{
    u8 v = func_80165658(o, D_80078EC1 & 0x7F) & 0x7F;
    return D_80078EB8 < v;
}
