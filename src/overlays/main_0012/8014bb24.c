/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern u8 D_80078E78[];
extern u16 D_80078EB4;
extern u8 func_80029178(s32 a);

void func_8014BB24(s32 unused, s32 d, s32 mode)
{
    u8 *st;
    u16 v;
    u16 n;

    st = D_80078E78;
    if (mode != 0) {
        D_80078EB4 = D_80078EB4 + d;
    } else if (func_80029178(0x1B) != 0) {
        v = D_80078EB4;
        if (v < 0x3E7) {
            n = v + d;
            D_80078EB4 = n;
            if (n > 0x3E7) {
                D_80078EB4 = 0x3E7;
            }
        }
    } else {
        v = D_80078EB4;
        if (v < 0x1F4) {
            n = v + d;
            D_80078EB4 = n;
            if (n > 0x1F4) {
                D_80078EB4 = 0x1F4;
            }
        }
    }
    if (*(u16 *)(st + 0x3A) < *(u16 *)(st + 0x3C)) {
        *(u16 *)(st + 0x3C) = *(u16 *)(st + 0x3A);
    }
}
