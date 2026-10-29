/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern u16 D_8011F82C;
extern u16 D_8011F82A;
extern s32 func_801783D0(s32 n, s32 shift);
extern void func_801778A8(u8 *p, s16 a, s32 b, s32 c);

void func_80176FF4(u8 *o)
{
    s32 v;

    o += 0x50;
    func_801778A8(o, func_801783D0(D_8011F82C, 4), 3, 0xFF);
    o += 0x3C;
    func_801778A8(o, func_801783D0(D_8011F82A, 4), 3, 0xFF);
    o += 0x3C;
    *(u16 *)(o + 0xC) = (u32)(D_8011F82C * 10355) >> 15;
    o += 0x14;
    v = (D_8011F82A * 20710) >> 16;
    *(u16 *)(o + 0xC) = v;
    *(u16 *)(o + 0x20) = v + 2;
}
