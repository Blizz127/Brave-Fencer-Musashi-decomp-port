/* Owner re-derivation from retail assembly (clean input set). */
/* Accumulates a tick counter (wrapping at 102 into a seconds counter capped at 9999999) and reports whole elapsed units through func_8002A2D4. */
#include "psx_types.h"

extern u32 D_80078F04;
extern u32 D_80078F08;
extern u32 D_80078F0C;
extern s32 func_80029504(void);
extern void func_8002A2D4(s32 n);

void func_80029ED4(u32 add) {
    u32 ticks;
    s32 delta;
    u32 secs;

    if (func_80029504() >= 10) {
        ticks = D_80078F04 + add;
        D_80078F04 = ticks;
        if (ticks >= 0x66) {
            D_80078F08 = D_80078F08 + 1;
            D_80078F04 = ticks % 102;
            if (D_80078F08 > 0x98967F) {
                D_80078F08 = 0x98967F;
            }
            secs = D_80078F08;
            delta = secs - D_80078F0C;
            if (delta >= 10) {
                D_80078F0C = secs;
                func_8002A2D4(delta / 10);
            }
        }
    }
}
