/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 1 if func_80034B98() returns nonzero, else 0. */
#include "psx_types.h"

extern s32 func_80034B98(void);

s32 func_8001BD80(void) {
    return func_80034B98() != 0;
}
