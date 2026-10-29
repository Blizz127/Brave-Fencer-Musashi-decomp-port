/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_80029178(0x7F) and returns its result truncated to a byte. */
#include "psx_types.h"

extern s32 func_80029178(s32 a);

u8 func_80029104(void) {
    return func_80029178(0x7F);
}
