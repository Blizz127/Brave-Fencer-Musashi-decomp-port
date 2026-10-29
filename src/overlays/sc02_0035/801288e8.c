/* Owner re-derivation from retail assembly (clean input set). */
/* Sets D_800C7C60 = 0x60 and D_800C7C64 = &D_800A2E20, then tail-forwards func_8001ABBC's result
 * (mode 0 request using D_800AF558, the value of D_801BC3F0, and &D_800C7C60). */
#include "psx_types.h"

extern s32 D_800C7C60;
extern s32 D_800C7C64;
extern s32 D_801BC3F0;
extern u8 D_800AF558[];
extern s32 D_800A2E20;
extern s32 func_8001ABBC(s32 a0, s32 a1, void *a2, s32 a3, s32 a4);

s32 func_801288E8(void)
{
    s32 *p = &D_800C7C60;

    *p = 0x60;
    D_800C7C64 = (s32)&D_800A2E20;
    return func_8001ABBC(0, 0, D_800AF558, D_801BC3F0, (s32)p);
}
