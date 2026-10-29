/* Owner re-derivation from retail assembly (clean input set). */
/* If the D_801274D4 hook is installed, tail-calls it with the object; otherwise returns whether the
 * squared distance from (a0+4) to D_80126CAC is at least D_801274E0. */
#include "psx_types.h"

extern u32 D_801274D4; /* guest function pointer: s32 (*)(void *) */
extern s32 D_801274E0;
extern s16 D_80126CAC[];
extern s32 func_80013478(s16 *a, s16 *b);

s32 func_8012C044(void *a0)
{
    if (D_801274D4 != 0) {
        return ((s32 (*)(void *))D_801274D4)(a0);
    }
    return func_80013478((s16 *)((u8 *)a0 + 4), D_80126CAC) >= D_801274E0;
}
