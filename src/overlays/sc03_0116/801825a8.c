#include "stage_252_light.h"

void func_801825A8(Actor252 *actor)
{
    D_800A5EA8 = D_801950BC;
    D_800A5EA8.z = (s32)((u32)func_8004787C(actor->angle) * 10u) / 4096 - 5;
    actor->angle = ((u16)actor->angle + 113u) & 4095u;
    func_80028620(2, &D_800A5EA8);
}
