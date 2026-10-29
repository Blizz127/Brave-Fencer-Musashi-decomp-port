/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Installs a new stream-end callback; if one is pending it is run first and the swap only happens when it returns non-zero. */
#include "psx_types.h"

typedef s32 (*StreamCallback)(void);

extern u8 D_8006AEF4;
extern u32 D_800BA0F8[]; /* guest StreamCallback */

s32 func_80037CD8(StreamCallback callback) {
    if ((D_8006AEF4 & 2) && D_800BA0F8[0] != 0) {
        if (((StreamCallback)D_800BA0F8[0])() != 0) {
            D_800BA0F8[0] = (u32)callback;
            D_8006AEF4 &= ~2;
            return 1;
        }
        return 0;
    }
    D_800BA0F8[0] = (u32)callback;
    D_8006AEF4 &= ~2;
    return 1;
}
