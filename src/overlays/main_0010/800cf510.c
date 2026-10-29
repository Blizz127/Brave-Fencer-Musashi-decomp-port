/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: resets the screen state (func_80014928, flag D_800B9A11, func_8001C0C8, func_80015310, func_8001B384, D_801152BA = 0), then runs func_80011CFC when D_80127084 is set, else func_80011D54(3). */
#include "psx_types.h"

extern void func_80014928(void);
extern void func_8001C0C8(void);
extern void func_80015310(void);
extern void func_8001B384(void);
extern void func_80011CFC(void);
extern void func_80011D54(s32 a);
extern u8 D_800B9A11;
extern s16 D_801152BA;
extern s32 D_80127084;

void func_800CF510(void)
{
    func_80014928();
    D_800B9A11 = 1;
    func_8001C0C8();
    func_80015310();
    func_8001B384();
    D_801152BA = 0;
    if (D_80127084 != 0) {
        func_80011CFC();
    } else {
        func_80011D54(3);
    }
}
