/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s16 D_80126CB8;
extern s16 D_80126CB4;
extern s32 func_8004CFEC(s32 a, s32 b);

s32 func_8012BDBC(u8 *o, s32 k) {
    s32 a;
    s32 d;
    a = func_8004CFEC(*(s16 *)(o + 0xE) - D_80126CB8, D_80126CB4 - *(s16 *)(o + 6));
    d = (((a - 0x400) & 0xFFF) - *(s16 *)(*(u32 *)(o + 0x20) + 0x12)) & 0xFFF;
    if (d < 0x800) {
        return d < (s16)k;
    }
    return (0x1000 - d) < (s16)k;
}
