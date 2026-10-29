/* Owner re-derivation from retail assembly (clean input set). */
/* Clears a global word, calls a helper with zero arguments and returns 0. */
#include "psx_types.h"

extern s32 D_800A4F24;
extern void func_8003EDE8(s32 a, s32 b, s32 c);

s32 func_8002D8D4(void) {
    D_800A4F24 = 0;
    func_8003EDE8(0, 0, 0);
    return 0;
}
