#include "psx_types.h"

extern s32 D_801270D0[];
extern u8 D_8018A868[];
extern s32 func_8012C51C(void *placement, u8 *owner);

void func_8017F240(void)
{
    if (D_801270D0[0] == 0) {
        func_8012C51C(D_8018A868, 0);
        D_801270D0[0] = 1;
    }
}
