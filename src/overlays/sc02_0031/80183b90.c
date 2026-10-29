/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 func_800D0CE0(void);

s32 func_80183B90(void) {
    return func_800D0CE0() != 0;
}
