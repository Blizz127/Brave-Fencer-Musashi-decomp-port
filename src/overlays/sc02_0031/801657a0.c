/* Owner re-derivation from retail assembly (clean input set). */
/* One-shot latch: if D_80078EC3 is still clear, sets it and runs func_801657D8() once. No caller
 * found in any available region/overlay; body never sets v0. */
#include "psx_types.h"

extern u8 D_80078EC3;
extern void func_801657D8(void);

void func_801657A0(void)
{
    if (D_80078EC3 == 0) {
        D_80078EC3 = 1;
        func_801657D8();
    }
}
