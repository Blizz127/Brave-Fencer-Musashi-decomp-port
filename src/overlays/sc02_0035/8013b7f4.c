/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 D_801BEA3C;
extern void func_8013B83C(s32, s32, s32);

void func_8013B7F4(s32 a0, s32 a1) {
    func_8013B83C(a0, a1, D_801BEA3C);
}
