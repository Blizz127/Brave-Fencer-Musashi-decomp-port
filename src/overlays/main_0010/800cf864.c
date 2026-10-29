/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 1 while the mode word is 0x3069 or D_80126B58 is 0x1A; otherwise the result of func_800CF8B4. */
#include "psx_types.h"

extern s32 func_800CF8B4(void);
extern s16 D_800B9A08;
extern u16 D_80126B58;

s32 func_800CF864(void)
{
    if (D_800B9A08 != 0x3069) {
        if (D_80126B58 != 0x1A) {
            return func_800CF8B4();
        }
        return 1;
    }
    return 1;
}
