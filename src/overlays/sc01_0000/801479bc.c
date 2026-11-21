/* Fresh isolated derivation: wrapping add/negate and inclusive signed bounds. */
#include "psx_types.h"

void func_801479BC(char *object, s32 delta, s32 limit)
{
    s32 sum = (s32)(*(u32 *)(object + 0x28) + (u32)delta);
    *(u32 *)(object + 0x28) = (u32)sum;
    if (delta < 0) {
        s32 lower = (s32)(0U - (u32)limit);
        if (sum <= lower) {
            *(u32 *)(object + 0x28) = (u32)lower;
            return;
        }
    }
    if (delta > 0 && *(s32 *)(object + 0x28) >= limit) {
        *(u32 *)(object + 0x28) = (u32)limit;
    }
}
