#include "forest_actor_mask.h"

u32 func_80172608(ForestPlayerState *actor)
{
    actor->status_mask |= 0x40u;
    return actor->status_mask;
}
