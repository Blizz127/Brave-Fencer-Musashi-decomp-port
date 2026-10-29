/* Owner re-derivation from retail assembly (clean input set). */
/* Returns angle `from` advanced by the turn step from func_80012B04, wrapped to 12 bits. */
#include "psx_types.h"

extern s32 func_80012B04(s32, s32, s32);

s32 func_80012ABC(s32 from, s16 to, s16 rate) {
    return (from + func_80012B04((s16)from, to, rate)) & 0xFFF;
}
