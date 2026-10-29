/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_8017B0E4(void *a0, s32 a1);
extern u16 D_801B7504;
extern void func_8012A4BC(void);
extern u16 D_801B7A98;

void func_8017BCF4(void *a0) {
    s16 v;

    func_8017B0E4(a0, 6);
    D_801B7504 = D_801B7504 + 1;
    v = D_801B7504;
    if (v >= 0x19) {
        func_8012A4BC();
        D_801B7A98 = 0;
    }
}
