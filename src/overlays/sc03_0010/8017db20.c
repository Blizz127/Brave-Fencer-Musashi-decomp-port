#include "steamwood_valve.h"

void func_8017DB20(struct ValvePlayer *player)
{
    if (func_800291B4(206) < D_80183BC5[player->selector])
        func_80171A04(player);
    else
        func_80171A10(player);
}
