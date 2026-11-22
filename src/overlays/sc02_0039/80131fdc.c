#include "mine_door_phase.h"

s32 func_80131FDC(MineDoorActor *actor)
{
    return D_8017F684[actor->phase](actor);
}
