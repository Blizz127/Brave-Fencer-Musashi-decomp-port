/* Owner re-derivation from retail assembly (clean input set). */
/* Finds a free slot (16-byte stride) in a 32-entry table, fills it from the source object, and returns whether one was found. */
#include "psx_types.h"

extern u8 D_800AF630[];

s32 func_80018450(u8 *src, s32 val) {
    s32 found = 0;
    s32 i;

    for (i = 0; i < 0x20; i++) {
        u8 *p = D_800AF630 + i * 16;

        if (*(s16 *)(p + 0xA1A8) == 0) {
            *(s16 *)(p + 0xA1A8) = 1;
            *(s32 *)(p + 0xA1AC) = val;
            *(u8 *)(p + 0xA1B5) = 0;
            *(u16 *)(p + 0xA1B0) = *(u16 *)(src + 0x28);
            *(u16 *)(p + 0xA1B2) = *(u16 *)(src + 0x2A);
            *(u8 *)(p + 0xA1B4) = src[0x27];
            found = 1;
            break;
        }
    }
    return found;
}
