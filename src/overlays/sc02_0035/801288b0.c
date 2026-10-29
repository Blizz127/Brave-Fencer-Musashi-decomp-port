/* Owner re-derivation from retail assembly (clean input set). */
/* Tail-forwards func_8001ABBC's result (mode 0 request using D_800AEFD0 and the value of D_801BC3F0). */
#include "psx_types.h"

extern s32 D_801BC3F0;
extern u8 D_800AEFD0[];
extern s32 func_8001ABBC(s32 a0, s32 a1, void *a2, s32 a3, s32 a4);

s32 func_801288B0(void)
{
    return func_8001ABBC(0, 0, D_800AEFD0, D_801BC3F0, 0);
}
