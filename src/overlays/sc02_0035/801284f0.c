/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern u8 D_800B9A64;
extern void func_80175308(void);
extern void func_8016E8F0(void);
extern void func_80175494(void);
extern void func_80011C10(void);

void func_801284F0(void)
{
    func_80175308();
    func_8016E8F0();
    if (D_800B9A64 != 0) {
        func_80175494();
    }
    func_80011C10();
}
