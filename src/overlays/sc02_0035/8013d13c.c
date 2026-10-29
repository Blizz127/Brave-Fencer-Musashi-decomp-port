/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_8013D9B0(s32 a);
extern s32 D_801BD6A4;

s32 func_8013D13C(s32 a) {
    func_8013D9B0(a);
    return D_801BD6A4;
}
