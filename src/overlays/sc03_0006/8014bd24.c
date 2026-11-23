#include "bincho_burst.h"

extern u16 D_80078EB6;
extern u16 D_80078EB8;

/* The rescue refills current BP after raising the maximum. */
void func_8014BD24(void *player, s32 amount)
{
    (void)player;
    D_80078EB8 += (u32)amount;
    if (D_80078EB8 > D_80078EB6) {
        D_80078EB8 = D_80078EB6;
    }
}
