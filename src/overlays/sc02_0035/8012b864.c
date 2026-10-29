/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern s16 D_80126CB8;
extern s16 D_80126CB4;
s32 func_8004CFEC(s32 a, s32 b);

s32 func_8012B864(s16 *o) {
    return (func_8004CFEC(o[7] - D_80126CB8, D_80126CB4 - o[3]) - 0x400) & 0xFFF;
}
