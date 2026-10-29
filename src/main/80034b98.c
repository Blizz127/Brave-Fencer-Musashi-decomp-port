/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the stream status: 0 idle, 1 busy, 2/4 for the pending flags, 8 for a latched error (which it clears). */
#include "psx_types.h"

extern s32 D_8006AEF8;
extern s32 D_8006AEFC;
extern u8 D_8007620C;
extern u8 D_80076214;

s32 func_80034B98(void) {
    if (D_8006AEF8 != D_8006AEFC) {
        if (D_80076214 != 0) {
            goto error;
        }
        if (D_8007620C & 0x80) {
            return 2;
        }
        if (D_8007620C & 0x20) {
            return 4;
        }
        return 1;
    }
    D_8007620C = 0;
    if (D_80076214 == 0) {
        return 0;
    }
error:
    D_80076214 = 0;
    return 8;
}
