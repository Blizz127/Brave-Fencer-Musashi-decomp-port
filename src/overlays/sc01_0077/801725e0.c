#include "forest_actor_mask.h"

u32 func_801725E0(ForestPlayerState *actor)
{
    actor->status_mask |= 0x10u;
    return actor->status_mask;
}
