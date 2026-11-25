#include "forest_actor_mask.h"

u32 func_801725CC(ForestPlayerState *actor)
{
    actor->status_mask |= 0x8u;
    return actor->status_mask;
}
