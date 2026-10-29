/* func_80154ED8 [80154ED8,80154F9C) - owner re-derivation from retail assembly (clean input set). */
/* Stores the request flags at +0xC8, sets or clears bit 24 from global input state, and posts the matching request via func_801550FC. */
#include "psx_types.h"

/* callers read v0: no (the only caller, 80154E4C, overwrites it). */
extern s16 D_80078E96;
extern s16 D_80078E9E;
extern s16 D_80078EA6;
extern u8 D_800D8D10[];
extern void func_801550FC(u32 req);

void func_80154ED8(u8 *o, u32 flags)
{
    *(u32 *)(o + 0xC8) = flags;
    if (D_80078E96 & 0x8000) {
        *(u32 *)(o + 0xC8) = flags | 0x1000000;
        func_801550FC((u32)D_800D8D10);
    } else if ((D_80078E9E < 0x10 || D_80078EA6 < 0x10) && (*(u32 *)(o + 0x44) & 0xC0) == 0) {
        *(u32 *)(o + 0xC8) = flags | 0x1000000;
        func_801550FC(0x40000002);
    } else {
        *(u32 *)(o + 0xC8) &= 0xC0FFFFFF;
        func_801550FC(flags);
    }
}
