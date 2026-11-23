#include "bincho_burst.h"

extern u16 D_80078EB6;

/* A rescued villager increases maximum BP, up to the game's 1,634 limit. */
void func_8014BCEC(void *player, s32 amount)
{
    (void)player;
    D_80078EB6 += (u32)amount;
    if (D_80078EB6 > 1634) {
        D_80078EB6 = 1634;
    }
}
