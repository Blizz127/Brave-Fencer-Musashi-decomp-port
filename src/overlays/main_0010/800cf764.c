/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: when func_800D0214 accepts the value read by func_80128218, clears D_80127084 and sets D_80114E98. */
#include "psx_types.h"

extern s32 func_80128218(void);
extern s32 func_800D0214(s32 a);
extern s32 D_80127084;
extern s32 D_80114E98;

void func_800CF764(void)
{
    if (func_800D0214(func_80128218()) != 0) {
        D_80127084 = 0;
        D_80114E98 = 1;
    }
}
