/* Owner re-derivation from retail assembly (clean input set). */
/* Runs two update helpers unless a global byte flag equals 1. */
#include "psx_types.h"

extern u8 D_800B9A11;
extern void func_8001F9F8(void);
extern void func_8001D1C4(void);

void func_8001C00C(void) {
    if (D_800B9A11 != 1) {
        func_8001F9F8();
        func_8001D1C4();
    }
}
