#include "forest_actor_mask.h"

u32 func_801725F4(ForestPlayerState *actor)
{
    actor->status_mask |= 0x20u;
    return actor->status_mask;
}
