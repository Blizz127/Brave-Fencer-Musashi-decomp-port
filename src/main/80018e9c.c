/* Owner re-derivation from retail assembly and m2c-assisted drafting.
 * Matched 33/33 words using gcc-2.7.2-psx -O2. */
#include "psx_types.h"
void func_80028D78(s32);
void func_800291A0(s32, s32);
s32 func_800291B4(s32);
extern u8 D_80078D98[];
extern u16 D_80078DCA;
void func_80018E9C(void *arg0) {
    s32 var_s0;
    if (arg0 == D_80078D98) {
        if (D_80078DCA & 0x100) {
            var_s0 = func_800291B4(0xE2) + 1;
            if ((u32)(var_s0 & 0xFF) >= 0x5AU) {
                var_s0 = 0;
                func_80028D78(1);
            }
            func_800291A0(0xE2, var_s0 & 0xFF);
        } else {
            func_800291A0(0xE2, 0);
        }
    }
}
