#include "forest_actor_mask.h"

u32 func_801725A4(ForestPlayerState *actor)
{
    actor->status_mask |= 0x2u;
    return actor->status_mask;
}
