/* Owner re-derivation from retail assembly (clean input set). */
/* Writes the decimal digits of a value as glyph codes, optional separator, -1-terminated. */
#include "psx_types.h"

extern s16 D_80063870[];
extern s16 D_80063858[];
extern s16 D_8006386C;

void func_800242D0(s16 value, s16 *out, s32 flags) {
    s16 n;
    s16 digit;
    s16 v;

    n = flags & 0xFFAF;
    flags &= 0x80;
    v = value;
    if (flags) {
        n = 4;
    }
    while (--n >= 0) {
        digit = v / D_80063870[n];
        if (digit != 0) {
            v -= digit * D_80063870[n];
            *out = D_80063858[digit];
        } else {
            *out = D_80063858[0];
        }
        out++;
        if (flags && n == 2) {
            *out++ = D_8006386C;
        }
    }
    *out = -1;
}
