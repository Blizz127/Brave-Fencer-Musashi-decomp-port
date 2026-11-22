/* Reset the fixed player's phase before installing its next callback. */
#include "mine_award_controller.h"

extern MineAwardPhase D_80127088;
extern u32 D_801270BC;

void func_80174684(MineAwardPhase phase)
{
    D_80126B58[0x214] = 0;
    D_80127088 = phase;
    D_801270BC = 0;
}
