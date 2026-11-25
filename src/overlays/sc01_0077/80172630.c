#include "forest_actor_mask.h"

u32 func_80172630(ForestPlayerState *actor)
{
    actor->status_mask |= 0x100u;
    return actor->status_mask;
}
