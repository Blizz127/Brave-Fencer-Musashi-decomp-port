/* Owner re-derivation from retail assembly (clean input set). */
/* Returns a random value in [0, range) using func_8005C49C, or 0 when range is 0. */
#include "psx_types.h"

extern s32 func_8005C49C(void);

s32 func_80029CD4(s32 range) {
    if (range == 0) {
        return 0;
    }
    return func_8005C49C() % range;
}
