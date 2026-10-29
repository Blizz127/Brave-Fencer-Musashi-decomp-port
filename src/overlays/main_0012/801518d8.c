/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

s32 func_801518D8(u8 *o) {
    u8 t = o[0x3E];
    if (t == 0x1B || (t == 0x1C || (t == 0xF || (t == 0x10 || (t == 3 || t == 0x13))))) {
        return 1;
    }
    return t == 0x17;
}
