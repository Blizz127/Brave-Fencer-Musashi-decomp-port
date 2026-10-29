/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u32 func_80144A04(u16 *p);
extern void func_801599A4(u8 *o);
extern void func_80159B3C(u8 *o);
extern void func_8016EDEC(u32 x, s32 v);

int func_80149F2C(u8 *o, u16 *p) {
    u32 x;
    if (*p == 0x83 || *p == 0x61) {
        x = func_80144A04(p);
        if (x != 0) {
            func_801599A4(o);
            func_80159B3C(o);
            func_8016EDEC(x, 0x1000000);
            return 1;
        }
    }
    return 0;
}
