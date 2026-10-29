/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u8 func_80029178(s32 a);
extern s32 D_801AAE88[];

s32 func_80184844(void) {
    s32 count = 0;
    s32 *p = D_801AAE88;

    while (1) {
        if (func_80029178(*p) == 0) {
            return 0;
        }
        count = count + 1;
        if (count >= 0x26) {
            return 1;
        }
        p = p + 1;
    }
}
