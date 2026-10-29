/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u32 func_8012913C(u16);
extern void func_80129350(u32, u32);

u32 func_80132EF4(u32 a, u16 b) {
    u32 r = func_8012913C(b);
    if (r != 0) {
        func_80129350(r, a);
    }
    return r;
}
