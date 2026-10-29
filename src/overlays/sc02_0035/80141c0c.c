/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u16 D_80115110;
extern u16 D_80185824[];
extern void func_8002D4C8(s32 a0, s32 a1);

void func_80141C0C(s32 a0) {
    s32 term;
    s32 idx;

    term = ((s16)a0) << 1;
    idx = term + D_80115110;
    func_8002D4C8(D_80185824[idx], 0);
}
