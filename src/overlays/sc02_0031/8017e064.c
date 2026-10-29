/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u16 D_801888FC;
extern u16 D_801888FA;
extern u16 D_801888F8;
extern void func_80162CCC(void);

void func_8017E064(void) {
    s16 v = D_801888FC - 8;

    D_801888FC = v;
    D_801888FA = v;
    D_801888F8 = v;
    if (v < -0x80) {
        func_80162CCC();
    }
}
