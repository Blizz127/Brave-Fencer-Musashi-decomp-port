/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

s32 func_80029178(s32);                         /* extern */

s32 func_80028EF0(void) {
    return func_80029178(0x11) & 0xFF;
}
