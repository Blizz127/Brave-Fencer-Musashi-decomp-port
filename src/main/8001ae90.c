/* Owner re-derivation from retail assembly (clean input set). */
/* Compares a value against a global word depending on a mode selector. */
#include "psx_types.h"

extern s32 D_800747F4;

s32 func_8001AE90(s32 arg0, s32 arg1, s32 *arg2) {
    s32 ret;

    switch (arg0) {
    case 0:
    case 2:
        ret = *arg2 == D_800747F4;
        break;
    case 1:
        ret = arg1 == D_800747F4;
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}
