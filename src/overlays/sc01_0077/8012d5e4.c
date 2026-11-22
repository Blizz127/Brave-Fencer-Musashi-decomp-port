/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern s32 func_8012DEB8(s32 a, s32 b, s32 c);
extern u16 D_80126B98;

s32 func_8012D5E4(s32 a, s32 b, s32 c, s32 d)
{
    if (func_8012DEB8(a, b, c)) {
        D_80126B98 = d;
    } else {
        return 0;
    }
    return 1;
}
