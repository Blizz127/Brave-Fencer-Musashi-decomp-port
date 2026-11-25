#include "forest_actor_mask.h"

u32 func_80172694(const ForestPlayerState *actor)
{
    return actor->status_mask & 0x20u;
}
