/* Owner re-derivation from retail assembly (clean input set). */
/* Forwards the low byte of its argument to a helper with constant 0xC. */
#include "psx_types.h"

extern void func_80029124(s32 arg0, s32 arg1);

void func_80028DBC(u8 arg0) {
    func_80029124(0xC, arg0);
}
