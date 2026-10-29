/* Owner re-derivation from retail assembly (clean input set). */
/* Writes n glyph halfwords to out: one per nibble of val, high nibble first, each looked up in the glyph pointer table D_80182540. While zs is non-zero a leading zero nibble prints glyph 10 (blank) instead; the first non-zero nibble clears zs. */
#include "psx_types.h"

extern u32 D_80182540[];

void func_8017DCB0(u32 val, s16 n, u16 *out, s16 zs)
{
    s16 i;
    u32 g;
    u8 d;

    for (i = 0; i < n; i++) {
        g = val >> 28;
        d = g;
        if (zs != 0) {
            if (g != 0) {
                zs = 0;
            } else {
                d = 10;
            }
        }
        val <<= 4;
        *out++ = *(u16 *)D_80182540[d];
    }
}
