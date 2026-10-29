/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_8012C658(0x1B, b, a). */
#include "psx_types.h"

extern s32 func_8012C658(int a, int b, int c);

s32 func_80143994(int a, s16 b) {
    return func_8012C658(0x1B, b, a);
}
