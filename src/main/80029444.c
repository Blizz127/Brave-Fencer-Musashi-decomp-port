/* Owner re-derivation from retail assembly (clean input set). */
/* Ticks a frame/second/minute/hour play clock, carrying upward and saturating at 99:59:59. */
#include "psx_types.h"

extern u8 D_80078E7C;
extern u8 D_80078E7D;
extern u8 D_80078E7E;
extern u8 D_80078E7F;

void func_80029444(void) {
    D_80078E7C++;
    if (D_80078E7C >= 0x1E) {
        D_80078E7C = 0;
        D_80078E7D++;
        if (D_80078E7D >= 0x3C) {
            D_80078E7D = 0;
            D_80078E7E++;
            if (D_80078E7E >= 0x3C) {
                D_80078E7E = 0;
                D_80078E7F++;
                if (D_80078E7F >= 0x64) {
                    *(u32 *)&D_80078E7C = 0x633B3B00;
                }
            }
        }
    }
}
