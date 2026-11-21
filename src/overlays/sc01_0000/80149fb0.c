/* Own-retail sc01_0000 record and RAM state contract; native mapping unproved. */
#include "psx_types.h"
extern s32 D_801807C0[];
extern s16 D_800B9A08;
extern u8 D_80078E78[];
s32 func_80149FB0(u8 *object)
{
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    register u8 *base __asm__("$6") = D_80078E78;
#else
    u8 *base = D_80078E78;
#endif
    s32 *list = D_801807C0;
    s32 entry;
    while ((entry = *list++) != 0) {
        if (D_800B9A08 == entry) return 0;
    }
    if (*(u32 *)(object + 0x44) & 0x400) return 0;
    if (*(s16 *)(object + 0x1C8) != 0) return 0;
    if (*(u16 *)(object + 0xAC) & 0x100) {
        base[0x48] ^= 0x80;
        return 1;
    }
    return 0;
}
