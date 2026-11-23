#include "steamwood_valve.h"

void func_80171A1C(struct ValvePlayer *player)
{
    ++player->cursor;
    player->step = 0;
    player->substep = 0;
}
