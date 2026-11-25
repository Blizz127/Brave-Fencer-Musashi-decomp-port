#include "forest_actor_mask.h"

u32 func_801725B8(ForestPlayerState *actor)
{
    actor->status_mask |= 0x4u;
    return actor->status_mask;
}
