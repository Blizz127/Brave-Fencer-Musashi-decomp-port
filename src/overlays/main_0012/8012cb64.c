/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

s32 func_8012CB64(s16 *p, s32 a1, s32 a2, s32 a3, s32 a4) {
    if (p[0] >= a1 && p[0] < a2 && p[2] >= a3 && p[2] < a4) {
        return 1;
    }
    return 0;
}
