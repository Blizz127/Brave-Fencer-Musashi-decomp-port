/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: unless func_8001AF04 reports state 2, runs func_800CFDA4 when func_800CFE60 reports a hit, then clears D_80127084 and sets D_80114E98. */
#include "psx_types.h"

extern void func_800191A8(void);
extern s32 func_8001AF04(void);
extern s32 func_800CFE60(void);
extern void func_800CFDA4(void);
extern s32 D_80127084;
extern s32 D_80114E98;

void func_800CF7A4(void)
{
    func_800191A8();
    if (func_8001AF04() != 2 && func_800CFE60() != 0) {
        func_800CFDA4();
        D_80127084 = 0;
        D_80114E98 = 1;
    }
}
