/* Owner re-derivation from retail assembly and locally generated m2c. */
#include "psx_types.h"

extern s32 func_80029178(s32);

s32 func_80028F78(void) {
    return func_80029178(0x13) & 0xFF;
}
