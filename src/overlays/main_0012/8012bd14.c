/* Owner re-derivation from retail assembly (clean input set). */
/* Squared distance from the object's position (at +4) to the reference point D_80126B5C. */
#include "psx_types.h"

extern s32 D_80126B5C[];

extern s32 func_80013350(void *a, void *b);

s32 func_8012BD14(u8 *obj)
{
    return func_80013350(obj + 4, D_80126B5C);
}
