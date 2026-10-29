/* Owner re-derivation from retail assembly (clean input set). */
/* Returns angle `from` advanced by the scaled turn step from func_80012E0C, wrapped to 12 bits. */
#include "psx_types.h"

extern s32 func_80012E0C(s32, s32, s32, s32);

s32 func_80012DBC(s32 from, s16 to, s16 num, s16 den) {
    return (from + func_80012E0C((s16)from, to, num, den)) & 0xFFF;
}
