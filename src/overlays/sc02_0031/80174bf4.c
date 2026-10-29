/* Owner re-derivation from retail assembly (clean input set). */
/* Resets several fields to fixed constants, then dispatches a0 through D_801882E8[kind]. */
#include "psx_types.h"

extern void (*D_801882E8[])(u8 *);

void func_80174BF4(u8 *a0)
{
    u8 kind = *(u8 *)(a0 + 4);

    *(s32 *)(a0 + 0xC) = 0x12C;
    *(s32 *)(a0 + 0x14) = 0x12C;
    *(u16 *)(a0 + 0x20) = 0x155;
    *(u16 *)(a0 + 0x22) = 0x800;
    *(u16 *)(a0 + 0x24) = 0;
    *(u16 *)(a0 + 0x2E) = 0;
    *(s16 *)(a0 + 0x30) = -0x28;
    *(u16 *)(a0 + 0x32) = 0;
    *(s32 *)(a0 + 0x54) = 0;
    D_801882E8[kind](a0);
}
