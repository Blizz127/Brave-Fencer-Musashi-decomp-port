/* Owner re-derivation from retail assembly (clean input set). */
/* Stops the current voice, starts sound `id` on a new handle and records whether it is still busy. */
#include "psx_types.h"

extern s16 D_800A46A2;
extern s16 D_800A46A0;
extern u8 D_800A46B0;
extern s32 D_800A469C;
extern s16 D_800A46D2;
extern s32 D_800652F0[];
extern void func_8002ED90(void);
extern void func_800415A8(s32 handle);
extern s32 func_80041A80(s32 arg, s32 mode, s32 sound);
extern s32 func_800419B0(s32 handle);

s32 func_8002FDE8(s32 id, s32 arg1)
{
    func_8002ED90();
    if (D_800A46A2 >= 0) {
        func_800415A8(D_800A46A2);
        D_800A46A2 = -1;
    }
    D_800A46A0 = id | 0x4000;
    D_800A46B0 = 1;
    D_800A469C = *(s32 *)((u8 *)D_800652F0 + id * 4);
    D_800A46D2 = func_80041A80(arg1, -1, D_800A469C);
    D_800A46A2 = D_800A46D2;
    if (D_800A46A2 == -1) {
        D_800A46B0 = 1;
        return 1;
    }
    if ((s16)func_800419B0(D_800A46A2) == -1) {
        func_800415A8(D_800A46D2);
        D_800A46A2 = -1;
        D_800A46B0 = 1;
        return 1;
    }
    D_800A46B0 = 0;
    return 1;
}
