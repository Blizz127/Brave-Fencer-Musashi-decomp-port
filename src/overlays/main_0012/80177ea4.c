/* Owner re-derivation from retail assembly (clean input set). */
/* Builds seven 0x14-byte sprite packets in a row at p: the tag of each packet links it to the previous one, the first is the leading marker cell (digit code 0), the last is the closing cell (digit code 1) and the middle ones take one hex digit each from the top nibble of val (shifted left by 4 per cell), with leading zeros marked by the suppression flag 0x01000000 until the first non-zero digit. Returns the address after the last packet. The three overwritten assignments to d at the top of the loop only keep the loop's RTL size at the retail count, so that just the 0xFFFFFF mask is hoisted (byte match). */
#include "psx_types.h"

u32 *func_80177EA4(u32 *p, s32 val, u16 x, s32 y)
{
    s16 i;
    s16 d;
    u32 flag;
    s32 ys;
    s16 m;

    flag = 0x100 << 16;
    for (i = 0; i < 7; i++) {
        d = val >> 1;
        d = val >> 2;
        d = val >> 3;
        p[0] = (((u32)p - 0x14) & 0xFFFFFF) | 0x03000000;
        ys = y << 16;
        p[1] = 0x74808080;
        if (i != 0) {
            if (i == 6) {
                d = 1;
                flag = 0;
            } else {
                d = ((val >> 16) & 0xF) + 1;
                val <<= 4;
            }
            m = d;
            if (m != 1) {
                flag = 0;
            }
            p[2] = ys | (u16)x | flag;
        } else {
            d = 0;
            p[2] = ys | (u16)x;
        }
        p[3] = (d << 3) | 0x6B564000;
        p += 5;
        x += 8;
    }
    return p;
}
