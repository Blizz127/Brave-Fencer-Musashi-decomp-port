/* Owner re-derivation from retail assembly (clean input set). */
/* Streams the next chunk for the current entry and advances the read cursor by the request result. */
/* One function [80030538,80030634): splat's func_80030604 is this function's default case and epilogue. */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    s16 f2;
    u8 pad4[0xC];
    u8 f10;
    u8 pad11[7];
} E24;

extern E24 D_800A4640[];
extern s32 D_800A46C0;
extern s32 D_800A46C4;
extern u16 D_800A46CC;
extern s16 D_800A46CE;
extern s32 func_80042154(s32, s32, s32);

s32 func_80030538(void)
{
    s16 i = D_800A46CE;

    switch ((s16)func_80042154(D_800A46C0 + D_800A46C4, 0x8000, D_800A4640[i].f2)) {
    case -2:
        D_800A46C4 += 0x8000;
        D_800A46CC++;
        goto ret0;
    case -1:
        D_800A4640[i].f10 = 1;
        D_800A46CC += 2;
        return 1;
    default:
        {
            u16 *p = &D_800A46CC;
            *p += 2;
        }
    ret0:
        return 0;
    }
}
