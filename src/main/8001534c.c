/* Owner re-derivation from retail assembly (clean input set). */
/* Computes a record pointer from an index and forwards the call with the remaining arguments. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern void func_800153A4(void *rec, s32 a, s16 b, s16 c, s32 d, s32 e);

void func_8001534C(s16 arg0, s32 arg1, s16 arg2, s16 arg3, u8 arg4, u8 arg5) {
    u8 *base;
    s32 off;

    base = D_800AF630;
    off = (arg0 << 4) + 0x9DA8;
    func_800153A4(base + off, arg1, arg2, arg3, arg4, arg5);
}
