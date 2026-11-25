#include "forest_actor_mask.h"

u32 func_80172590(ForestPlayerState *actor)
{
    actor->status_mask |= 0x1u;
    return actor->status_mask;
}
